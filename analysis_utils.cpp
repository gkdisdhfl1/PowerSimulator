#include "analysis_utils.h"
#include "config.h"
#include <complex>
#include <QDebug>
#include <QValueAxis>
#include <QLabel>

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

    struct CycleAccumulators {
        std::array<double, 3> totalVoltageRmsSumSq = {0.0};
        std::array<double, 3> totalCurrentRmsSumSq = {0.0};
        std::array<double, 3> totalVoltageRmsSumSq_ll = {0.0}; // [0]:ab [1]:bc [2]:ca
        std::array<double, 3> activePowerSum = {0.0};
        std::array<double, 3> fundVoltageRmsSumSq = {0.0};
        std::array<double, 3> fundCurrentRmsSumSq = {0.0};
        std::array<double, 3> fundVoltageRmsSumSq_ll = {0.0};
        double residualVoltageRmsSum = 0.0;
        double residualCurrentRmsSum = 0.0;
        double residualVoltageFundamentalSum = 0.0;
        double residualCurrentFundamentalSum = 0.0;

        // A상 기준
        double dominantVoltageRmsSumSq = 0.0;
        double dominantCurrentRmsSumSq = 0.0;
    };

    CycleAccumulators accumulateCycleData(const std::vector<MeasuredData>& cycleBuffer, int dominantVoltageOrder, int dominantCurrentOrder) {
        CycleAccumulators acc;

        // Phasor 3개의 복소수 합을 계산하는 헬퍼 함수
        auto accumulatePhase = [&](int index, double v_rms, double v_ll_rms, double i_rms, double p_active, const auto& fv, const auto& fv_ll, const auto& fi) {
            acc.totalVoltageRmsSumSq[index] += v_rms * v_rms;
            acc.totalVoltageRmsSumSq_ll[index] += v_ll_rms * v_ll_rms;
            acc.totalCurrentRmsSumSq[index] += i_rms * i_rms;
            acc.activePowerSum[index] += p_active;
            acc.fundVoltageRmsSumSq[index] += fv.rms * fv.rms;
            acc.fundVoltageRmsSumSq_ll[index] += fv_ll.rms * fv_ll.rms;
            acc.fundCurrentRmsSumSq[index] += fi.rms * fi.rms;
        };
        auto sumPhasor = [&](const GenericPhaseData<HarmonicAnalysisResult>& phasePhasors) {
            std::complex<double> sum(0, 0);
            sum += std::complex<double>(phasePhasors.a.phasorX, phasePhasors.a.phasorY);
            sum += std::complex<double>(phasePhasors.b.phasorX, phasePhasors.b.phasorY);
            sum += std::complex<double>(phasePhasors.c.phasorX, phasePhasors.c.phasorY);
            return sum;
        };

        for(const auto& data : cycleBuffer) {
            accumulatePhase(0, data.voltageRms.a, data.voltageRms_ll.ab, data.currentRms.a, data.activePower.a, data.fundamentalVoltage.a, data.fundamentalVoltage_ll.ab, data.fundamentalCurrent.a);
            accumulatePhase(1, data.voltageRms.b, data.voltageRms_ll.bc, data.currentRms.b, data.activePower.b, data.fundamentalVoltage.b, data.fundamentalVoltage_ll.bc, data.fundamentalCurrent.b);
            accumulatePhase(2, data.voltageRms.c, data.voltageRms_ll.ca, data.currentRms.c, data.activePower.c, data.fundamentalVoltage.c, data.fundamentalVoltage_ll.ca, data.fundamentalCurrent.c);

            acc.residualVoltageRmsSum += data.residualVoltageRms;
            acc.residualCurrentRmsSum += data.residualCurrentRms;

            // 복소수 합의 절대값 계산 (잔류 기본파)
            acc.residualVoltageFundamentalSum += std::abs(sumPhasor(data.fundamentalVoltage));
            acc.residualCurrentFundamentalSum += std::abs(sumPhasor(data.fundamentalCurrent));

            if(dominantVoltageOrder > 1 && data.dominantVoltage.a.order == dominantVoltageOrder) {
                acc.dominantVoltageRmsSumSq += data.dominantVoltage.a.rms * data.dominantVoltage.a.rms;
            }
            if(dominantCurrentOrder > 1 && data.dominantCurrent.a.order == dominantCurrentOrder) {
                acc.dominantCurrentRmsSumSq += data.dominantCurrent.a.rms * data.dominantCurrent.a.rms;
            }
        }
        return acc;
    }

    void calculatePhaseMetrics(OneSecondSummaryData& summary, const CycleAccumulators& acc, size_t N) {
        std::array<double, 3> voltageRms, currentRms, pActive, apparent, powerFactor, voltageThd, currentThd, reactive_arr;
        std::array<double, 3> voltageRms_ll, voltageThd_ll;

        for(int i{0}; i < 3; ++i) {
            voltageRms[i] = std::sqrt(acc.totalVoltageRmsSumSq[i] / N);
            voltageRms_ll[i] = std::sqrt(acc.totalVoltageRmsSumSq_ll[i] / N);
            currentRms[i] = std::sqrt(acc.totalCurrentRmsSumSq[i] / N);
            pActive[i] = acc.activePowerSum[i] / N;

            apparent[i] = voltageRms[i] * currentRms[i];
            powerFactor[i] = (apparent[i] > 1e-9) ? std::abs(pActive[i] / apparent[i]) : 0.0;

            // 무효 전력
            double reactive_sq_arg = apparent[i] * apparent[i] - pActive[i] * pActive[i];
            reactive_arr[i] = (reactive_sq_arg > 0.0) ? std::sqrt(reactive_sq_arg) : 0.0;

            // THD (THD = harmonicRMS / fundamentalRMS)
            const double fundVoltageRms = std::sqrt(acc.fundVoltageRmsSumSq[i] / N);
            const double fundVoltageRms_ll = std::sqrt(acc.fundVoltageRmsSumSq_ll[i] / N);
            const double fundCurrentRms = std::sqrt(acc.fundCurrentRmsSumSq[i] / N);

            if(fundVoltageRms > 1e-9) {
                double harmonicVoltageRmsSq = (voltageRms[i] * voltageRms[i]) - (fundVoltageRms * fundVoltageRms);
                voltageThd[i] = (harmonicVoltageRmsSq > 1e-9) ? (std::sqrt(harmonicVoltageRmsSq) / fundVoltageRms) * 100.0 : 0.0;
            } else {
                voltageThd[i] = (voltageRms[i] > 1e-9) ? std::numeric_limits<double>::infinity() : 0.0;
            }

            if(fundCurrentRms > 1e-9) {
                double harmonicCurrentRmsSq = (currentRms[i] * currentRms[i]) - (fundCurrentRms * fundCurrentRms);
                currentThd[i] = (harmonicCurrentRmsSq > 1e-9) ? (std::sqrt(harmonicCurrentRmsSq) / fundCurrentRms) * 100.0 : 0.0;
            } else {
                currentThd[i] = (currentRms[i] > 1e-9) ? std::numeric_limits<double>::infinity() : 0.0;
            }

            if(fundVoltageRms_ll > 1e-9) {
                double harmonicVoltageRmsSq_ll = (voltageRms_ll[i] * voltageRms_ll[i]) - (fundVoltageRms_ll * fundVoltageRms_ll);
                voltageThd_ll[i] = (harmonicVoltageRmsSq_ll > 1e-9) ? (std::sqrt(harmonicVoltageRmsSq_ll) / fundVoltageRms_ll) * 100.0 : 0.0;
            } else {
                // 전압값이 0이라면 THD 값은 의미 없음
                voltageThd_ll[i] = (voltageRms_ll[i] > 1e-9) ? std::numeric_limits<double>::infinity() : 0.0;
            }
        }

        // 결과 할당
        summary.totalVoltageRms = {voltageRms[0], voltageRms[1], voltageRms[2]};
        summary.totalVoltageRms_ll = {voltageRms_ll[0], voltageRms_ll[1], voltageRms_ll[2]};
        summary.totalCurrentRms = {currentRms[0], currentRms[1], currentRms[2]};
        summary.activePower = {pActive[0], pActive[1], pActive[2]};
        summary.apparentPower = {apparent[0], apparent[1], apparent[2]};
        summary.powerFactor = {powerFactor[0], powerFactor[1], powerFactor[2]};
        summary.voltageThd = {voltageThd[0], voltageThd[1], voltageThd[2]};
        summary.voltageThd_ll = {voltageThd_ll[0], voltageThd_ll[1], voltageThd_ll[2]};
        summary.currentThd = {currentThd[0], currentThd[1], currentThd[2]};
        summary.reactivePower = {reactive_arr[0], reactive_arr[1], reactive_arr[2]};
    }

    void calculateTotalMetrics(OneSecondSummaryData& summary, const std::vector<MeasuredData>& cycleBuffer, const CycleAccumulators& acc, size_t N) {
        summary.totalActivePower = summary.activePower.a + summary.activePower.b + summary.activePower.c;
        summary.totalApparentPower = summary.apparentPower.a + summary.apparentPower.b + summary.apparentPower.c;
        summary.totalReactivePower = summary.reactivePower.a + summary.reactivePower.b + summary.reactivePower.c;
        summary.totalPowerFactor = (summary.totalApparentPower > 1e-6) ? std::abs(summary.totalActivePower) / summary.totalApparentPower : 0.0;

        // 주파수 계산 (2샘플 필요)
        if(cycleBuffer.size() >= 2) {
            double duration = std::chrono::duration<double>(
                                  cycleBuffer.back().timestamp - (cycleBuffer.end() - 2)->timestamp).count();
            summary.frequency = cycleBuffer.back().fundamentalVoltage.a.order * (1.0 / duration);
        } else {
            summary.frequency = 0.0;
        }

        summary.residualVoltageRms = acc.residualVoltageRmsSum / N;
        summary.residualCurrentRms = acc.residualCurrentRmsSum / N;
        summary.residualVoltageFundamental = acc.residualVoltageFundamentalSum / N;
        summary.residualCurrentFundamental = acc.residualCurrentFundamentalSum / N;

        summary.dominantHarmonicVoltageRms = std::sqrt(acc.dominantVoltageRmsSumSq / N);
        summary.dominantHarmonicCurrentRms = std::sqrt(acc.dominantCurrentRmsSumSq / N);
    }

    double calculateNemaUnbalance(const PhaseData& rms) {
        const double avg = (rms.a + rms.b + rms.c) / 3.0;
        if(avg < 1e-9) return 0.0;
        const double max_dev = std::max({std::abs(rms.a - avg),
                                         std::abs(rms.b - avg),
                                         std::abs(rms.c - avg)});
        return (max_dev / avg) * 100.0;
    }
    double calculateNemaUnbalance(const LineToLineData& rms) {
        const double avg = (rms.ab + rms.bc + rms.ca) / 3.0;
        if(avg < 1e-9) return 0.0;
        const double max_dev = std::max({std::abs(rms.ab - avg),
                                         std::abs(rms.bc - avg),
                                         std::abs(rms.ca - avg)});
        return (max_dev / avg) * 100.0;
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

std::vector<HarmonicAnalysisResult> AnalysisUtils::convertSpectrumToHarmonics(const std::vector<std::complex<double>>& spectrum)
{
    std::vector<HarmonicAnalysisResult> results;
    if(spectrum.empty()) {
        qWarning() << "AnalysisUtils::convertSpectrumToHarmonics() Spectrum is emtpy!!";
        return results;
    }

    // 0차(DC) 성분 추가
    HarmonicAnalysisResult dc_result;
    dc_result.order = 0;
    dc_result.rms = std::abs(spectrum[0].real());
    dc_result.phase = 0.0;
    dc_result.phasorX = spectrum[0].real();
    dc_result.phasorY = 0.0;
    results.push_back(dc_result);

    // 1차부터 나머지 고조파 성분 추가
    for(int order{1}; order < spectrum.size(); ++order) {
        results.push_back(createHarmonicResult(spectrum, order));
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

LineToLineData AnalysisUtils::calculateTotalRms_ll(const std::vector<DataPoint>& samples)
{
    if(samples.empty())
        return {};

    double sumSq_ab = 0.0;
    double sumSq_bc = 0.0;
    double sumSq_ca = 0.0;

    for(const auto& sample : samples) {
        sumSq_ab += sample.voltage_ll.ab * sample.voltage_ll.ab;
        sumSq_bc += sample.voltage_ll.bc * sample.voltage_ll.bc;
        sumSq_ca += sample.voltage_ll.ca * sample.voltage_ll.ca;
    }

    const double n = static_cast<double>(samples.size());
    return {
        std::sqrt(sumSq_ab / n),
        std::sqrt(sumSq_bc / n),
        std::sqrt(sumSq_ca / n)
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

    // 1. 기본 정보 설정
    summary.dominantHarmonicVoltageOrder = lastCycleData.dominantVoltage.a.order;
    summary.dominantHarmonicCurrentOrder = lastCycleData.dominantCurrent.a.order;
    summary.dominantHarmonicVoltagePhase = utils::radiansToDegrees(lastCycleData.dominantVoltage.a.phase);
    summary.dominantHarmonicCurrentPhase = utils::radiansToDegrees(lastCycleData.dominantCurrent.a.phase);
    summary.fundamentalVoltage = lastCycleData.fundamentalVoltage;
    summary.fundamentalVoltage_ll = lastCycleData.fundamentalVoltage_ll;
    summary.fundamentalCurrent = lastCycleData.fundamentalCurrent;

    // 2. 데이터 누적
    CycleAccumulators acc = accumulateCycleData(cycleBuffer,
                                                summary.dominantHarmonicVoltageOrder,
                                                summary.dominantHarmonicCurrentOrder);

    // 3. 위상별 계산 (RMS, Power, THD)
    calculatePhaseMetrics(summary, acc, N);

    // 4. 전체 지표 계산 (Total Power, Frequency, Residual)
    calculateTotalMetrics(summary, cycleBuffer, acc, N);

    // 5. 불평형률 계산 (NEMA)
    summary.nemaVoltageUnbalance = calculateNemaUnbalance(summary.totalVoltageRms);
    summary.nemaVoltageUnbalance_ll = calculateNemaUnbalance(summary.totalVoltageRms_ll);
    summary.nemaCurrentUnbalance = calculateNemaUnbalance(summary.totalCurrentRms);

    // 6. 대칭 성분 및 불평형률 (U0, U2)
    auto LN_voltageData = lastCycleData.fundamentalVoltage;
    auto LL_voltageData = lastCycleData.fundamentalVoltage_ll;
    auto currentData = lastCycleData.fundamentalCurrent;

    summary.voltageSymmetricalComponents = calculateSymmetricalComponents(LN_voltageData.a, LN_voltageData.b, LN_voltageData.c);
    summary.currentSymmetricalComponents = calculateSymmetricalComponents(currentData.a, currentData.b, currentData.c);
    auto sym_ll_temp = calculateSymmetricalComponents(LL_voltageData.ab, LL_voltageData.bc, LL_voltageData.ca);
    summary.voltageSymmetricalComponents_ll.positive = sym_ll_temp.positive;
    summary.voltageSymmetricalComponents_ll.negative = sym_ll_temp.negative;

    auto calculateSymUnbalance = [](const SymmetricalComponents& sym, double& u0, double& u2) {
        if(sym.positive.magnitude > 1e-9) {
            u0 = (sym.zero.magnitude / sym.positive.magnitude) * 100.0;
            u2 = (sym.negative.magnitude / sym.positive.magnitude) * 100.0;
        } else {
            u0 = (sym.zero.magnitude > 1e-9) ? std::numeric_limits<double>::infinity() : 0.0;
            u2 = (sym.negative.magnitude > 1e-9) ? std::numeric_limits<double>::infinity() : 0.0;
        }
    };
    calculateSymUnbalance(summary.voltageSymmetricalComponents, summary.voltageU0Unbalance, summary.voltageU2Unbalance);
    calculateSymUnbalance(summary.currentSymmetricalComponents, summary.currentU0Unbalance, summary.currentU2Unbalance);

    // 7. 마지막 사이클 고조파 정보 복사
    summary.lastCycleVoltageHarmonics = lastCycleData.voltageHarmonics;
    summary.lastCycleCurrentHarmonics = lastCycleData.currentHarmonics;

    summary.lastCycleFullVoltageHarmonics = lastCycleData.fullVoltageHarmonics;
    summary.lastCycleFullCurrentHarmonics = lastCycleData.fullCurrentHarmonics;

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

SymmetricalComponents AnalysisUtils::calculateSymmetricalComponents(const HarmonicAnalysisResult& p1, const HarmonicAnalysisResult& p2, const HarmonicAnalysisResult& p3)
{
    SymmetricalComponents result;

    // 1. 3상 기본파 페이저를 복소수로 변환
    const std::complex<double> V_a(p1.phasorX, p1.phasorY);
    const std::complex<double> V_b(p2.phasorX, p2.phasorY);
    const std::complex<double> V_c(p3.phasorX, p3.phasorY);

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


ScaleUnit AnalysisUtils::updateScaleUnit(double range)
{
    qDebug() << "range: " << range;
    if(range < 1.0) return ScaleUnit::Milli;
    if(range >= 1000.0) return ScaleUnit::Kilo;
    return ScaleUnit::Base;
}

double AnalysisUtils::scaleValue(double value, ScaleUnit unit)
{
    // qDebug() << "value: " << value;
    switch(unit)
    {
    case ScaleUnit::Milli:
        // qDebug() << "unit: Milli";
        return value * 1000.0;
    case ScaleUnit::Kilo:
        // qDebug() << "unit: Kilo";
        return value / 1000.0;
    default:
        // qDebug() << "unit: default";
        return value;
    }
}

ScaleUnit AnalysisUtils::updateAxis(QValueAxis* axis, QLabel* label, int scaleIndex, bool isVoltage)
{
    if(!axis || !label) return ScaleUnit::Base;
    qDebug() << "updateAxis in";

    double newRange = config::View::RANGE_TABLE[scaleIndex];
    qDebug() << "newRange: " << newRange;
    ScaleUnit unit = updateScaleUnit(newRange);

    double displayRange = scaleValue(newRange, unit);
    qDebug() << "displayRange: " << displayRange;
    qDebug() << "---------------------------------------";
    axis->setRange(-displayRange, displayRange);

    const char* baseUnit = isVoltage ? "V" : "A";
    QString unitString;
    if(unit == ScaleUnit::Milli)
        unitString = QString("m%1").arg(baseUnit);
    else if(unit == ScaleUnit::Kilo)
        unitString = QString("k%1").arg(baseUnit);
    else
        unitString = baseUnit;

    label->setText(QString("[%1]").arg(unitString));

    return unit;
}
