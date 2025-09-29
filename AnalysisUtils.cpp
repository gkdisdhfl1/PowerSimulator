#include "AnalysisUtils.h"
#include "config.h"
#include "trigonometric_table.h"
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

std::map<int, std::unique_ptr<TrigonometricTable>> AnalysisUtils::m_trigTableCache;

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

std::vector<std::complex<double>> AnalysisUtils::calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow)
{
    const size_t N = samples.size();
    if(N == 0) {
        return {};
    }

    // 캐시에서 테이블을 가져옴
    const auto* trigTable = getTrigonometricTable(N);
    if(!trigTable) {
        qWarning() << "스펙트럼 계산 중단. " << N << "이 없음";
        return {};
    }

    const int max_k = N / 2;

    // 임시 데이터 벡터 생성
    std::vector<double> processed_wave(N);

    for(int i = 0; i < N; ++i)
        processed_wave[i] = samples[i].voltage;

    if(useWindow) {
        // Hann 윈도우 적용
        const double two_pi_over_N_minus_1 = config::Math::TwoPi / (N - 1);
        for(size_t i = 0; i < N; ++i)
            processed_wave[i] *= 0.5 * (1.0 - std::cos(i * two_pi_over_N_minus_1));
    }

    // DFT 실행
    std::vector<std::complex<double>> spectrum(max_k + 1);

    for(size_t k = 0; k <= max_k; ++k) {
        double real_sum = 0.0;
        double imag_sum = 0.0;

        for(size_t n = 0; n < N; ++n) {
            real_sum += processed_wave[n] * trigTable->getCos(k, n);
            imag_sum -= processed_wave[n] * trigTable->getSin(k, n);
        }

        // DFT 정규화
        const double normFactor = std::sqrt(2.0) / N;
        spectrum[k] = {normFactor * real_sum, normFactor * imag_sum};
    }
    return spectrum;
}

std::vector<double> AnalysisUtils::generateFundamentalWave(const std::vector<DataPoint>& samples)
{
    const size_t N = samples.size();
    if(N < 2) {
        return {};
    }

    // 1. 스펙트럼 계산
    auto spectrum = calculateSpectrum(samples, true);
    if(spectrum.empty()) {
        return {};
    }

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
        return {}; // 유의미한 주파수를 못찾음
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
    {
        std::vector<HarmonicAnalysisResult> results;
        if(spectrum.size() < 2) return results;

        // 기본파는 항상 결과에 추가
        results.push_back(createHarmonicResult(spectrum, 1));

        // 나머지 중에서 가장 큰 고조파를 찾음
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

        // 노이즈와 구분하기 위한 최소 임계값
        // 기본파 RMS의 1%보다 커야 함
        const double fundamentalRmsSq = std::norm(spectrum.size() > 1 ? spectrum[1] : 0);
        const double noiseThresholdSq = fundamentalRmsSq * 0.0001;

        // 유의미한 고조파 결과 추가
        if(harmonicOrder != -1 && maxHarmonicMagSq > noiseThresholdSq) {
            results.push_back(createHarmonicResult(spectrum, harmonicOrder));
        }

        return results;
    }
}

void AnalysisUtils::precomputeTables(const std::vector<int>& N_values)
{
    m_trigTableCache.clear();
    qDebug() << "테이블 미리 계산";
    for(int N : N_values) {
        if(N > 0 && m_trigTableCache.find(N) == m_trigTableCache.end()) {
            const int max_k = N / 2;
            m_trigTableCache[N] = std::make_unique<TrigonometricTable>(N, max_k);
        }
    }
}

const TrigonometricTable* AnalysisUtils::getTrigonometricTable(int N)
{
    if(N <= 0) {
        qWarning() << "잘못된 "<< N << "에 대한 테이블 요청됨";
        return nullptr;
    }

    auto it = m_trigTableCache.find(N);
    if(it != m_trigTableCache.end()) {
        return it->second.get();
    }

    // 테이블이 캐시에 없으면 , 동적으로 생성하고 추가
    qDebug() << N << "에 대한 테이블 동적 생성";
    const int max_k = N / 2;
    auto new_table = std::make_unique<TrigonometricTable>(N, max_k);
    const TrigonometricTable* table_ptr = new_table.get();
    m_trigTableCache[N] = std::move(new_table);

    return table_ptr;
}
