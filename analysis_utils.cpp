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
    const auto& lastCycleData = cycleBuffer.back();
    const size_t N = cycleBuffer.size();

    summary.dominantHarmonicVoltageOrder = lastCycleData.dominantVoltage[0].order;
    summary.dominantHarmonicCurrentOrder = lastCycleData.dominantCurrent[0].order;

    // 1. 모든 누적 변수를 std::array로 선언
    std::array<double, 3> totalVoltageRmsSumSq = {0.0};
    std::array<double, 3> totalCurrentRmsSumSq = {0.0};
    std::array<double, 3> activePowerSum = {0.0};
    std::array<double, 3> fundVoltageRmsSumSq = {0.0};
    std::array<double, 3> fundCurrentRmsSumSq = {0.0};
    double residualVoltageRmsSum = 0.0;
    double residualCurrentRmsSum = 0.0;

    // A상 기준
    double dominantVoltageRmsSumSq = 0.0; double dominantCurrentRmsSumSq = 0.0;

    // 2. for 루프를 사용하여 모든 3상 값 누적
    for(const auto& data : cycleBuffer) {
        totalVoltageRmsSumSq[0] += data.voltageRms.a * data.voltageRms.a;
        totalVoltageRmsSumSq[1] += data.voltageRms.b * data.voltageRms.b;
        totalVoltageRmsSumSq[2] += data.voltageRms.c * data.voltageRms.c;

        totalCurrentRmsSumSq[0] += data.currentRms.a * data.currentRms.a;
        totalCurrentRmsSumSq[1] += data.currentRms.b * data.currentRms.b;
        totalCurrentRmsSumSq[2] += data.currentRms.c * data.currentRms.c;

        activePowerSum[0] += data.activePower.a;
        activePowerSum[1] += data.activePower.b;
        activePowerSum[2] += data.activePower.c;

        residualVoltageRmsSum += data.residualVoltageRms;
        residualCurrentRmsSum += data.residualCurrentRms;

        fundVoltageRmsSumSq[0] += data.fundamentalVoltage[0].rms * data.fundamentalVoltage[0].rms;
        fundVoltageRmsSumSq[1] += data.fundamentalVoltage[1].rms * data.fundamentalVoltage[1].rms;
        fundVoltageRmsSumSq[2] += data.fundamentalVoltage[2].rms * data.fundamentalVoltage[2].rms;

        fundCurrentRmsSumSq[0] += data.fundamentalCurrent[0].rms * data.fundamentalCurrent[0].rms;
        fundCurrentRmsSumSq[1] += data.fundamentalCurrent[1].rms * data.fundamentalCurrent[1].rms;
        fundCurrentRmsSumSq[2] += data.fundamentalCurrent[2].rms * data.fundamentalCurrent[2].rms;

        if(summary.dominantHarmonicVoltageOrder > 1 && data.dominantVoltage[0].order == summary.dominantHarmonicVoltageOrder) {
            dominantVoltageRmsSumSq += data.dominantVoltage[0].rms * data.dominantVoltage[0].rms;
        }
        if(summary.dominantHarmonicCurrentOrder > 1 && data.dominantCurrent[0].order == summary.dominantHarmonicCurrentOrder) {
            dominantCurrentRmsSumSq += data.dominantCurrent[0].rms * data.dominantCurrent[0].rms;
        }
    }
    // 3. 최종 계산
    std::array<double, 3> voltageRms, currentRms, pActive, apparent, powerFactor, voltageThd, currentThd, reactive_arr;

    for(int i{0}; i < 3; ++i) {
        voltageRms[i] = std::sqrt(totalVoltageRmsSumSq[i] / N);
        currentRms[i] = std::sqrt(totalCurrentRmsSumSq[i] / N);
        pActive[i] = activePowerSum[i] / N;

        apparent[i] = voltageRms[i] * currentRms[i];
        powerFactor[i] = (apparent[i] > 1e-6) ? std::abs(pActive[i]) / apparent[i] : 0.0;

        // 잔류 계산
        double reactive = 0.0;
        const double reactive_sq_arg = apparent[i] * apparent[i] - pActive[i] * pActive[i];
        qDebug() << "apparent_arr[" << i << "] = " << voltageRms[i] << " * " << currentRms[i] << " = " << apparent[i];

        // 오차 처리
        if(reactive_sq_arg > 0.0)
            reactive = std::sqrt(reactive_sq_arg);
        // 음수일 경우 0으로 처리
        reactive_arr[i] = reactive;

        // THD 계산
        const double fundVoltageRms = std::sqrt(fundVoltageRmsSumSq[i] / N);
        const double fundCurrentRms = std::sqrt(fundCurrentRmsSumSq[i] / N);

        if(fundVoltageRms > 1e-6) {
            double harmonicVoltageRmsSq = (voltageRms[i] * voltageRms[i]) - (fundVoltageRms * fundVoltageRms);
            voltageThd[i] = (harmonicVoltageRmsSq > 0) ? (std::sqrt(harmonicVoltageRmsSq) / fundVoltageRms) * 100.0 : 0.0;
        } else {
            voltageThd[i] = 0.0;
        }

        if(fundCurrentRms > 1e-6) {
            double harmonicCurrentRmsSq = (currentRms[i] * currentRms[i]) - (fundCurrentRms * fundCurrentRms);
            currentThd[i] = (harmonicCurrentRmsSq > 0) ? (std::sqrt(harmonicCurrentRmsSq) / fundCurrentRms) * 100.0 : 0.0;
        } else {
            currentThd[i] = 0.0;
        }
    }

    // 4. NEMA 불평형률 계산
    auto calculateUnbalance = [](const std::array<double, 3>& rms_arr) {
        const double avg = (rms_arr[0] + rms_arr[1] + rms_arr[2]) / 3.0;
        if(avg < 1e-9) return 0.0;
        const double max_dev = std::max({std::abs(rms_arr[0] - avg),
                                         std::abs(rms_arr[1] - avg),
                                         std::abs(rms_arr[2] - avg)});
        return (max_dev / avg) * 100.0;
    };
    summary.nemaVoltageUnbalance = calculateUnbalance(voltageRms);
    summary.nemaCurrentUnbalance = calculateUnbalance(currentRms);

    // 4. 총계 전체 역률 계산
    double totalActivePower = pActive[0] + pActive[1] + pActive[2];
    double totalApparentPower = apparent[0] + apparent[1] + apparent[2];
    double totalReactivePower = reactive_arr[0] + reactive_arr[1] + reactive_arr[2];
    double totalPowerFactor = (totalApparentPower > 1e-6) ? std::abs(totalActivePower) / totalApparentPower : 0.0;

    // ----- system THD 계산 --------
    // 3상 전체 RMS 제곱의 합
    double totalRmsVoltageSqSum = (voltageRms[0] * voltageRms[0]) + (voltageRms[1] * voltageRms[1]) + (voltageRms[2] * voltageRms[2]);
    // 3상 기본파 RMS 제곱의 합
    double totalFundRmsVoltageSq = (fundVoltageRmsSumSq[0] / N) + (fundVoltageRmsSumSq[1] / N) + (fundVoltageRmsSumSq[2] / N);
    double systemVoltageThd = 0.0;

    if(totalFundRmsVoltageSq > 1e-12) {
        double totalHarmonicVoltageSqSum = totalRmsVoltageSqSum - totalFundRmsVoltageSq;
        if(totalHarmonicVoltageSqSum > 0) {
            systemVoltageThd = (std::sqrt(totalHarmonicVoltageSqSum) / std::sqrt(totalFundRmsVoltageSq)) * 100.0;
        }
    }
    double totalRmsCurrentSqSum = (currentRms[0] * currentRms[0]) + (currentRms[1] * currentRms[1]) + (currentRms[2] * currentRms[2]);
    // 3상 기본파 RMS 제곱의 합
    double totalFundRmsCurrentSq = (fundCurrentRmsSumSq[0] / N) + (fundCurrentRmsSumSq[1] / N) + (fundCurrentRmsSumSq[2] / N);
    double systemCurrentThd = 0.0;

    if(totalFundRmsCurrentSq > 1e-12) {
        double totalHarmonicCurrentSqSum = totalRmsCurrentSqSum - totalFundRmsCurrentSq;
        if(totalHarmonicCurrentSqSum) {
            systemCurrentThd = (std::sqrt(totalHarmonicCurrentSqSum) / std::sqrt(totalFundRmsCurrentSq)) * 100.0;
        }
    }
    // -------------------------

    // 5. 계산된 값들 구조체 할당
    summary.totalVoltageRms = {voltageRms[0], voltageRms[1], voltageRms[2]};
    summary.totalCurrentRms = {currentRms[0], currentRms[1], currentRms[2]};
    summary.activePower = {pActive[0], pActive[1], pActive[2]};
    summary.apparentPower = {apparent[0], apparent[1], apparent[2]};
    summary.powerFactor = {powerFactor[0], powerFactor[1], powerFactor[2]};
    summary.voltageThd = {voltageThd[0], voltageThd[1], voltageThd[2]};
    summary.currentThd = {currentThd[0], currentThd[1], currentThd[2]};
    summary.reactivePower = {reactive_arr[0], reactive_arr[1], reactive_arr[2]};

    summary.totalActivePower = totalActivePower;
    summary.totalApparentPower = totalApparentPower;
    summary.totalReactivePower = totalReactivePower;
    summary.totalPowerFactor = totalPowerFactor;

    summary.systemVoltageThd = systemVoltageThd;
    summary.systemCurrentThd = systemCurrentThd;
    summary.residualVoltageRms = residualVoltageRmsSum / N;
    summary.residualCurrentRms = residualCurrentRmsSum / N;

    // 6. 기존의 lastCycleData에서 가져오는 값들 할당
    summary.dominantHarmonicVoltageRms = std::sqrt(dominantVoltageRmsSumSq / N);
    summary.dominantHarmonicCurrentRms = std::sqrt(dominantCurrentRmsSumSq / N);

    summary.dominantHarmonicVoltagePhase = utils::radiansToDegrees(lastCycleData.dominantVoltage[0].phase);
    summary.dominantHarmonicCurrentPhase = utils::radiansToDegrees(lastCycleData.dominantCurrent[0].order);

    summary.fundamentalVoltagePhase = utils::radiansToDegrees(lastCycleData.fundamentalVoltage[0].phase);
    summary.fundamentalCurrentPhase = utils::radiansToDegrees(lastCycleData.fundamentalCurrent[0].phase);
    summary.fundamentalVoltageRms = lastCycleData.fundamentalVoltage[0].rms;
    summary.fundamentalCurrentRms = lastCycleData.fundamentalCurrent[0].rms;

    // 7. 대칭 성분 계산 및 할당
    summary.voltageSymmetricalComponents = calculateSymmetricalComponents(lastCycleData.fundamentalVoltage);
    summary.currentSymmetricalComponents = calculateSymmetricalComponents(lastCycleData.fundamentalCurrent);

    // 8. U0, U2 불평형률 계산
    const auto& voltageSym = summary.voltageSymmetricalComponents;
    if(voltageSym.positive.magnitude > 1e-9) {
        summary.voltageU0Unbalance = (voltageSym.zero.magnitude / voltageSym.positive.magnitude) * 100;
        summary.voltageU2Unbalance = (voltageSym.negative.magnitude / voltageSym.positive.magnitude) * 100;
    }
    const auto& currentSym = summary.voltageSymmetricalComponents;
    if(currentSym.positive.magnitude > 1e-9) {
        summary.voltageU0Unbalance = (currentSym.zero.magnitude / currentSym.positive.magnitude) * 100;
        summary.voltageU2Unbalance = (currentSym.negative.magnitude / currentSym.positive.magnitude) * 100;
    }
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

SymmetricalComponents AnalysisUtils::calculateSymmetricalComponents(const std::array<HarmonicAnalysisResult, 3>& fundamentals)
{
    SymmetricalComponents result;

    // 1. 3상 기본파 페이저를 복소수로 변환
    const std::complex<double> V_a(fundamentals[0].phasorX, fundamentals[0].phasorY);
    const std::complex<double> V_b(fundamentals[1].phasorX, fundamentals[1].phasorY);
    const std::complex<double> V_c(fundamentals[2].phasorX, fundamentals[2].phasorY);

    // 2. 회전 연산자 'a' 정의(a = 120도)
    const std::complex<double> a = std::polar(1.0, utils::degreesToRadians((120.0)));
    const std::complex<double> a_sq = a * a;

    // 3. 대칭 성분 계산
    const std::complex<double> V_zero = (V_a + V_b + V_c) / 3.0;
    const std::complex<double> V_positive = (V_a + a * V_b + a_sq * V_c) / 3.0;
    const std::complex<double> V_negative = (V_a + a_sq * V_b + a * V_c) / 3.0;

    // 4. 계산된 복소수를 크기/위상으로 변환하여 결과 구조체에 저장
    result.zero.magnitude = std::abs(V_zero);
    result.zero.phase_deg = utils::radiansToDegrees(std::arg(V_zero));

    result.positive.magnitude = std::abs(V_positive);
    result.positive.phase_deg = utils::radiansToDegrees(std::arg(V_positive));

    result.negative.magnitude = std::abs(V_negative);
    result.negative.phase_deg = utils::radiansToDegrees(std::arg(V_negative));
    return result;
}
