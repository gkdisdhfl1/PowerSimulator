#include "analysis_utils.h"
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

std::expected<std::vector<std::complex<double>>, AnalysisUtils::SpectrumError> AnalysisUtils::calculateSpectrum(const std::vector<DataPoint>& samples, DataType type, int phase, bool useWindow)
{
    if(samples.size() == 0) {
        qWarning() << "Input Data is empty!!!";
        return std::unexpected(SpectrumError::InvalidInput);
    }

    size_t N = samples.size();
    bool isOdd = (N % 2 != 0);
    if(isOdd)
        N += 1; // 짝수로 만듬

    // if(N % 2 != 0) {
    //     // qWarning() << "Invalid Input failed for N = " << N;
    //     return std::unexpected(SpectrumError::InvalidInput);
    // }

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
    auto get_value = [&](const DataPoint& p) {
        if(type == DataType::Voltage) {
            if(phase == 0) return p.voltage.a;
            if(phase == 1) return p.voltage.b;
            return p.voltage.c;
        } else { // Current
            if(phase == 0) return p.current.a;
            if(phase == 1) return p.current.b;
            return p.current.c;
        }
    };

    if(useWindow) {
        const double two_pi_over_N_minus_1 = config::Math::TwoPi / (N - 1);
        for(int i = 0; i < N; ++i) {
            fft_in[i] = get_value(samples[i]) * 0.5 * (1.0 - std::cos(i * two_pi_over_N_minus_1));
        }
    } else {
        for(int i = 0; i < N; ++i) {
            fft_in[i] = get_value(samples[i]);
        }
    }

    // 홀수였다면 마지막에 추가된 요소를 0으로 채움
    if(isOdd) {
        fft_in[N - 1] = 0.0;
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
    auto spectrumResult = calculateSpectrum(samples, DataType::Voltage, 0, true);
    if(!spectrumResult) {
        qWarning() << "Spectrum Calculation Failed generating Fundamental Wave!!!";
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

PhaseData AnalysisUtils::calculateActivePower(const std::vector<DataPoint>& samples)
{
    if(samples.empty())
        return {};

    double powerSum = 0.0;
    double powerSumB = 0.0;
    double powerSumC = 0.0;
    for(const auto& sample : samples) {
        powerSum += sample.voltage.a * sample.current.a;
        powerSumB += sample.voltage.b * sample.current.b;
        powerSumC += sample.voltage.c * sample.current.c;
    }

    const double n  = static_cast<double>(samples.size());
    return {
        powerSum / n,
        powerSumB / n,
        powerSumC / n
    };
}

PhaseData AnalysisUtils::calculateTotalRms(const std::vector<DataPoint>& samples, DataType type)
{
    if(samples.empty())
        return {};

    double sumSq = 0.0;
    double sumSqB = 0.0;
    double sumSqC = 0.0;
    if(type == DataType::Voltage) {
        for(const auto& sample : samples) {
            sumSq += sample.voltage.a * sample.voltage.a;
            sumSqB += sample.voltage.b * sample.voltage.b;
            sumSqC += sample.voltage.c * sample.voltage.c;
        }
    } else {
        for(const auto& sample : samples) {
            sumSq += sample.current.a * sample.current.a;
            sumSqB += sample.current.b * sample.current.b;
            sumSqC += sample.current.c * sample.current.c;
        }
    }

    const double n  = static_cast<double>(samples.size());
    return {
        std::sqrt(sumSq / n),
        std::sqrt(sumSqB / n),
        std::sqrt(sumSqC / n)
    };

}

OneSecondSummaryData AnalysisUtils::buildOneSecondSummary(const std::vector<MeasuredData>& cycleBuffer)
{
    if(cycleBuffer.empty()) {
        return {};
    }

    OneSecondSummaryData summary{};
    double sum_v_a = 0;
    const auto& lastCycleData = cycleBuffer.back();
    const size_t N = cycleBuffer.size();

    // 1. 마지막 사이클에서 A상 지배적 고조파의 차수와 위상을 결정
    summary.dominantHarmonicVoltageOrder = lastCycleData.dominantVoltage[0].order;
    summary.dominantHarmonicVoltagePhase = utils::radiansToDegrees(lastCycleData.dominantVoltage[0].phase);

    summary.dominantHarmonicCurrentOrder = lastCycleData.dominantCurrent[0].order;
    summary.dominantHarmonicCurrentPhase = utils::radiansToDegrees(lastCycleData.dominantCurrent[0].order);

    summary.fundamentalVoltagePhase = utils::radiansToDegrees(lastCycleData.fundamentalVoltage[0].phase);
    summary.fundamentalCurrentPhase = utils::radiansToDegrees(lastCycleData.fundamentalCurrent[0].phase);

    // 2. 전체 버퍼를 순회하며 RMS 값들의 제곱의 합과 유효 전력을 구함
    double totalVoltageRmsSumSq = 0.0; double totalCurrentRmsSumSq = 0.0; double activePowerSum = 0.0;
    double totalVoltageRmsSumSqB = 0.0; double totalCurrentRmsSumSqB = 0.0; double activePowerSumB = 0.0;
    double totalVoltageRmsSumSqC = 0.0; double totalCurrentRmsSumSqC = 0.0; double activePowerSumC = 0.0;

    // A상 기준
    double fundVoltageRmsSumSq = 0.0; double fundCurrentRmsSumSq = 0.0;
    double dominantVoltageRmsSumSq = 0.0; double dominantCurrentRmsSumSq = 0.0;

    for(const auto& data : cycleBuffer) {
        totalVoltageRmsSumSq += data.voltageRms.a * data.voltageRms.a;
        totalCurrentRmsSumSq += data.currentRms.a * data.currentRms.a;
        totalVoltageRmsSumSqB += data.voltageRms.b * data.voltageRms.b;
        totalCurrentRmsSumSqB += data.currentRms.b * data.currentRms.b;
        totalVoltageRmsSumSqC += data.voltageRms.c * data.voltageRms.c;
        totalCurrentRmsSumSqC += data.currentRms.c * data.currentRms.c;
        activePowerSum += data.activePower.a;
        activePowerSumB += data.activePower.b;
        activePowerSumC += data.activePower.c;
        fundVoltageRmsSumSq += data.fundamentalVoltage[0].rms * data.fundamentalVoltage[0].rms;
        fundCurrentRmsSumSq += data.fundamentalCurrent[0].rms * data.fundamentalCurrent[0].rms;
        if(summary.dominantHarmonicVoltageOrder > 1 && data.dominantVoltage[0].order == summary.dominantHarmonicVoltageOrder)
            dominantVoltageRmsSumSq += data.dominantVoltage[0].rms * data.dominantVoltage[0].rms;
        if(summary.dominantHarmonicCurrentOrder > 1 && data.dominantCurrent[0].order == summary.dominantHarmonicCurrentOrder)
            dominantCurrentRmsSumSq += data.dominantCurrent[0].rms * data.dominantCurrent[0].rms;
    }

    // 3. 최종 계산
    summary.totalVoltageRms.a = std::sqrt(totalVoltageRmsSumSq / N);
    summary.totalCurrentRms.a = std::sqrt(totalCurrentRmsSumSq / N);
    summary.totalVoltageRms.b = std::sqrt(totalVoltageRmsSumSqB / N);
    summary.totalCurrentRms.b = std::sqrt(totalCurrentRmsSumSqB / N);
    summary.totalVoltageRms.c = std::sqrt(totalVoltageRmsSumSqC / N);
    summary.totalCurrentRms.c = std::sqrt(totalCurrentRmsSumSqC / N);
    summary.activePower.a = activePowerSum / N;
    summary.activePower.b = activePowerSumB / N;
    summary.activePower.c = activePowerSumC / N;

    summary.fundamentalVoltageRms = std::sqrt(fundVoltageRmsSumSq / N);
    summary.fundamentalCurrentRms = std::sqrt(fundCurrentRmsSumSq / N);

    summary.dominantHarmonicVoltageRms = std::sqrt(dominantVoltageRmsSumSq / N);
    summary.dominantHarmonicCurrentRms = std::sqrt(dominantCurrentRmsSumSq / N);

    return summary;
}

double AnalysisUtils::calculateResidualRms(const std::vector<DataPoint>& samples, AnalysisUtils::DataType type)
{
    if(samples.empty()) {
        return 0.0;
    }

    double sum_sq = 0.0;

    if(type == AnalysisUtils::DataType::Voltage) {
        for(const auto& p : samples) {
            const double residualValue = p.voltage.a + p.voltage.b + p.voltage.c;
            sum_sq += residualValue * residualValue;
        }


    } else { // current
        for(const auto& p : samples) {
            const double residualValue = p.current.a + p.current.b + p.current.c;
            sum_sq += residualValue * residualValue;
        }
    }

    return std::sqrt(sum_sq / samples.size());
}

AdditionalMetricsData AnalysisUtils::calculateAdditionalMetrics(const MeasuredData& measuredData, const std::vector<DataPoint>& cycleBuffer)
{
    AdditionalMetricsData result;
    result.residualVoltageRms = calculateResidualRms(cycleBuffer, DataType::Voltage);
    result.residualCurrentRms = calculateResidualRms(cycleBuffer, DataType::Current);

    // --- 피상전력, 무효전력 계산 ---
    const std::array<double, 3> v_rms_arr = {measuredData.voltageRms.a, measuredData.voltageRms.b, measuredData.voltageRms.c};
    const std::array<double, 3> i_rms_arr = {measuredData.currentRms.a, measuredData.currentRms.b, measuredData.currentRms.c};
    const std::array<double, 3> p_active_arr = {measuredData.activePower.a, measuredData.activePower.b, measuredData.activePower.c};

    std::array<double, 3> apparent_arr;
    std::array<double, 3> reactive_arr;

    for(int i{0}; i < 3; ++i) {
        const double apparent = v_rms_arr[i] * i_rms_arr[i];
        apparent_arr[i] = apparent;
        qDebug() << "apparent_arr[" << i << "] = " << v_rms_arr[i] << " * " << i_rms_arr[i] << " = " << apparent_arr[i];

        double reactive = 0.0;
        const double reactive_sq_arg = apparent * apparent - p_active_arr[i] * p_active_arr[i];

        // 오차 처리
        if(reactive_sq_arg > 0.0)
            reactive = std::sqrt(reactive_sq_arg);
        // 음수일 경우 0으로 처리

        reactive_arr[i] = reactive;
        qDebug() << "reactive_arr[" << i << "] = sqrt(" << apparent << " * " << apparent << " - " << p_active_arr[i] << " * " << p_active_arr[i] << ") = " << reactive_arr[i];
        qDebug() << "--------------------------------";
    }

    result.apparentPower = {apparent_arr[0], apparent_arr[1], apparent_arr[2]};
    result.reactivePower = {reactive_arr[0], reactive_arr[1], reactive_arr[2]};

    return result;
}
