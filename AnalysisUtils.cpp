#include "AnalysisUtils.h"
#include "config.h"
#include <complex>
#include <QDebug>

namespace {
HarmonicAnalysisResult createHarmonicResult(const std::vector<std::complex<double>>& spectrum, int order)
{
    if(order <= 0 || order >= spectrum.size()) return {};

    const auto& phasorRms = spectrum[order];
    const double rms = std::abs(phasorRms);

    return {
        .order = order,
        .rms = rms,
        .phase = std::arg(phasorRms),
        .phasorX = phasorRms.real(),
        .phasorY = phasorRms.imag()
    };
}
}

std::map<int, kiss_fftr_cfg> AnalysisUtils::m_fftConfigCache;

const HarmonicAnalysisResult* AnalysisUtils::getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order)
{
    auto it = std::find_if(harmonics.begin(), harmonics.end(), [order](const HarmonicAnalysisResult& h) {
        return h.order == order;
    });
    return (it != harmonics.end()) ? &(*it) : nullptr;
}

double AnalysisUtils::calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp)
{
    if(v_comp && i_comp) {
        return v_comp->rms * i_comp->rms * std::cos(v_comp->phase - i_comp->phase);
    }
    return 0.0;
}

const HarmonicAnalysisResult* AnalysisUtils::getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics)
{
    const HarmonicAnalysisResult* dominant = nullptr;
    double maxRms = -1.0;

    for(const auto& h : harmonics) {
        if(h.order > 1 && h.rms > maxRms) {
            maxRms = h.rms;
            dominant = &h;
        }
    }
    return dominant;
}

std::expected<std::vector<std::complex<double>>, AnalysisUtils::SpectrumError> AnalysisUtils::calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow)
{
    const size_t N = samples.size();
    if(N == 0 || N % 2 != 0) {
        qWarning() << "Invalid Input failed for N = " << N;
        return std::unexpected(SpectrumError::InvalidInput);
    }

    // 1. FFT 설정 가져오기 (없으면 생성해서 캐시에 저장)
    if(m_fftConfigCache.find(N) == m_fftConfigCache.end()) {
        m_fftConfigCache[N] = kiss_fftr_alloc(N, 0, nullptr, nullptr);
    }
    kiss_fftr_cfg fft_cfg = m_fftConfigCache[N];
    if(!fft_cfg) {
        qWarning() << "kiss_fftr_alloc failed for N = " << N;
        return std::unexpected(SpectrumError::AllocationFailed);
    }

    // 2. 입력 데이터 준비 (Hann 윈도우 적용 포함)
    std::vector<kiss_fft_scalar> fft_in(N);
    if(useWindow) {
        const double two_pi_over_N_minus_1 = config::Math::TwoPi / (N - 1);
        for(int i = 0; i < N; ++i) {
            fft_in[i] = samples[i].voltage * 0.5 * (1.0 - std::cos(i * two_pi_over_N_minus_1));
        }
    } else {
        for(int i = 0; i < N; ++i) {
            fft_in[i] = samples[i].voltage;
        }
    }

    // 3. FFT 실행
    const int num_freq_bins = N / 2 + 1;
    std::vector<kiss_fft_cpx> fft_out(num_freq_bins);
    kiss_fftr(fft_cfg, fft_in.data(), fft_out.data());

    // 4. 결과 변환 및 정규화
    std::vector<std::complex<double>> spectrum(num_freq_bins);
    const double normFactor = std::sqrt(2.0) / N;

    // DC 성분(k = 0)
    spectrum[0] = {fft_out[0].r / static_cast<double>(N), 0.0}; // DC는 sqrt(2)로 나누지 않음

    for(int k = 1; k < num_freq_bins; ++k) {
        spectrum[k] = {normFactor * fft_out[k].r, normFactor * fft_out[k].i};
    }

    return spectrum;

}

std::expected<std::vector<double>, AnalysisUtils::WaveGenerateError> AnalysisUtils::generateFundamentalWave(const std::vector<DataPoint>& samples)
{
    const size_t N = samples.size();
    if(N < 2) {
        return std::unexpected(WaveGenerateError::NoSignificantFound);
    }

    // 1. 스펙트럼 계산
    auto spectrumResult = calculateSpectrum(samples, true);
    if(!spectrumResult) {
        return std::unexpected(WaveGenerateError::SpectrumCalculationFailed);
    }
    const auto& spectrum = *spectrumResult;

    // 2. 가장 강한 피크 찾기
    int fundamental_k = -1; // 기본 주파수 인덱스
    double max_magnitude_sq = -1.0;
    std::complex<double> fundamental_phasor = {0.0, 0.0};

    for(size_t k = 1; k < spectrum.size(); ++k) {
        double mag_sq = std::norm(spectrum[k]);
        if(mag_sq > max_magnitude_sq) {
            max_magnitude_sq = mag_sq;
            fundamental_k = k;
            fundamental_phasor = spectrum[k];
        }
    }

    if(fundamental_k == -1 || max_magnitude_sq < 1e-9) {
        return std::unexpected(WaveGenerateError::NoSignificantFound);
    }
    qDebug() << "finded fundamental_k = " << fundamental_k;
    // 3. 기본파 재구성
    std::vector<double> clean_wave;
    clean_wave.reserve(N);

    const double amplitude = std::abs(fundamental_phasor) * sqrt(2.0);
    const double phase = std::arg(fundamental_phasor);
    const double two_pi_over_N = config::Math::TwoPi / N;

    for(size_t n = 0; n < N; ++n) {
        double value = amplitude * std::cos(fundamental_k * two_pi_over_N * n + phase);
        clean_wave.push_back(value);
    }

    return clean_wave;
}

std::vector<HarmonicAnalysisResult> AnalysisUtils::findSignificantHarmonics(const std::vector<std::complex<double>>& spectrum) {
    std::vector<HarmonicAnalysisResult> results;
    if(spectrum.size() < 2) return results;

    // 1. 기본파는 항상 결과에 추가
    results.push_back(createHarmonicResult(spectrum, 1));
    const double px = results.back().phasorX;
    const double py = results.back().phasorY;
    const double fundamentalMagSq = px * px + py + py;

    // 2. 평균 노이즈 레벨 계산
    double noiseSumSq = 0.0;
    int noiseSampleCount = 0;
    for(size_t k = 2; k < spectrum.size(); ++k) {
        noiseSumSq += std::norm(spectrum[k]);
        ++noiseSampleCount;
    }
    const double averageNoiseMagSq = (noiseSampleCount > 0) ? (noiseSumSq / noiseSampleCount) : 0.0;

    // 3. 나머지 중에서 가장 큰 고조파를 찾음
    int harmonicOrder = -1;
    double maxHarmonicMagSq = -1.0;

    for(size_t k = 2; k < spectrum.size(); ++k) {
        double magSq = std::norm(spectrum[k]); // 크기의 제곱으로 비교
        if(magSq > maxHarmonicMagSq) {
            // 기존의 Fundamental을 harmonic으로 내림
            maxHarmonicMagSq= magSq;
            harmonicOrder = k;
        }
    }

    // 4. 동적 임계값 설정
    // - 최소한 평균 노이즈 레벨의 5배는 되어야 함
    // - 최소한 기본파 크기 제곱의 0.1배는 되어야함
    const double thresholdFromNoise = averageNoiseMagSq * 5.0;
    const double thresholdFromFundamental = fundamentalMagSq * 0.001;
    const double dynamicThresholdSq = std::max(thresholdFromNoise, thresholdFromFundamental);

    // 5. 찾은 피크가 동적 임계값을 넘는지 확인
    if(harmonicOrder != -1 && maxHarmonicMagSq > dynamicThresholdSq) {
        results.push_back(createHarmonicResult(spectrum, harmonicOrder));
    }

    return results;
}

double AnalysisUtils::calculateActivePower(const std::vector<DataPoint>& samples)
{
    if(samples.empty())
        return 0.0;

    double powerSum = 0.0;
    for(const auto& sample : samples) {
        powerSum += sample.voltage * sample.current;
    }

    return powerSum / samples.size();
}

double AnalysisUtils::calculateTotalRms(const std::vector<DataPoint>& samples, DataType type)
{
    if(samples.empty())
        return 0.0;

    double sumSq = 0.0;
    if(type == DataType::Voltage) {
        for(const auto& sample : samples) {
            sumSq += sample.voltage * sample.voltage;
        }
    } else {
        for(const auto& sample : samples) {
            sumSq += sample.current * sample.current;
        }
    }

    return std::sqrt(sumSq / samples.size());
}
