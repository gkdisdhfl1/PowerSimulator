#include "frequency_tracker.h"
#include "simulation_engine.h"
#include "AnalysisUtils.h"
#include <QDebug>

namespace {
    struct CoarseSearchConstants {
        static constexpr double Duration_S = 0.5; // 데이터 수집 시간
        static constexpr int MinSamples = 10;
        static constexpr double MinTrackingFreq = 1.0;
    };
    struct FllConstants {
        static constexpr double Kp = 0.45;
        static constexpr double Ki = 0.0001;
        static constexpr double Kd = 0.65;
        static constexpr double IntegralLimit = 10.0;
        static constexpr double OutputLimit = 1.0;
        static constexpr double FailureThresholdHz = 10.0;
        static constexpr int MaxFailCount = 10;
        static constexpr double LockThresholdHz = 0.05;
        static constexpr int MinLockCount = 10;
        static constexpr int MaxOscillations = 10;
    };
    struct PllConstants {
        static constexpr double FailureThresholdRad = 0.3;
        static constexpr double SevereFailureThresholdRad = 0.6;
        static constexpr int MaxFailCount = 5;
        static constexpr double Kp = 0.015;
        static constexpr double Ki = 0.000002;
        static constexpr double Kd = 0.48;
        static constexpr double IntegralActivationThresholdRad = 0.01;
        static constexpr double Integral_Limit = 10;
    };
    struct VerificationConstants {
        static constexpr int IntervalCycles = 200;
        static constexpr double Durationi_S = 0.2; // 샘플을 수집할 기간(초)
        static constexpr double ValidRatioMin = 0.49;
        static constexpr double ValidRatioMax = 1.27;
    };
}

FrequencyTracker::FrequencyTracker(SimulationEngine* engine, QObject *parent)
    : QObject{parent}
    , m_engine(engine)
    , m_trackingState(TrackingState::Idle)
    , m_coarseSearchSamplesNeeded(0)
    , m_isVerifying(false)
    , m_fll_failCounter(0)
    , m_fll_lockCounter(0)
    , m_fll_previousLfOutput(0.0)
    , m_fll_oscillationCounter(0)
    , m_pll_previousVoltagePhase(0.0)
    , m_pll_failCounter(0)
    , m_pll_cycleCounter(0)
{
    // FLL 컨트롤러 설정
    m_fllController.setCoefficients({ .Kp = FllConstants::Kp, .Ki = FllConstants::Ki, .Kd = FllConstants::Kd});
    m_fllController.setLimits(FllConstants::IntegralLimit, FllConstants::OutputLimit);

    // ZC 기본값 설정
    m_zcController.setCoefficients({ .Kp = PllConstants::Kp, .Ki = PllConstants::Ki, .Kd = PllConstants::Kd});
    m_zcController.setLimits(PllConstants::Integral_Limit, 1.0);

    m_zcController.setIntegralActivationThreshold(PllConstants::IntegralActivationThresholdRad);
}

// ==== public 함수 ==================
void FrequencyTracker::startTracking()
{
    qDebug() << "-- 추적 시작 --";
    resetAllStates();
    startCoarseSearch();
}

void FrequencyTracker::stopTracking()
{
    qDebug() << "-- 추적 끝 --";
    resetAllStates();
}

FrequencyTracker::TrackingState FrequencyTracker::currentState() const
{
    return m_trackingState;
}

void FrequencyTracker::process(const DataPoint& latestDataPoint, const MeasuredData& latestMeasuredData, const std::vector<DataPoint>& cycleBuffer)
{
    // 백그라운드 검증 데이터 수집
    if(m_isVerifying)
        processVerification(latestDataPoint);

    // 자동 추적 상태에 따른 분기 처리
    switch (m_trackingState) {
    case TrackingState::Coarse:
        processCoarseSearch(latestDataPoint);
        break;
    case TrackingState::FLL_Acquisition:
        // FLL은 매 사이클 데이터가 필요
        if(cycleBuffer.size() >= static_cast<size_t>(m_engine->parameters().samplesPerCycle)) {
            processFll(latestMeasuredData);
        }
        break;
    case TrackingState::FineTune:
        // 정밀 조정 상태일 때만 사이클 데이터 계산
        // 버퍼에 설정된 samplesPerCycle 만큼 데이터가 쌓이면 계산을 실행
        if(cycleBuffer.size() >= static_cast<size_t>(m_engine->parameters().samplesPerCycle)) {
            processFineTune(latestMeasuredData);
        }
        break;
    case TrackingState::Idle:
        if(cycleBuffer.size() >= static_cast<size_t>(m_engine->parameters().samplesPerCycle)) {
            break;
        }
        break;
    }
}

void FrequencyTracker::setFllCoefficients(const PidCoefficients& coeffs)
{
    m_fllController.setCoefficients(coeffs);
}
void FrequencyTracker::setZcCoefficients(const PidCoefficients& coeffs)
{
    m_zcController.setCoefficients(coeffs);
}
FrequencyTracker::PidCoefficients FrequencyTracker::getFllCoefficients()
{
    return m_fllController.getCoefficients();
}
FrequencyTracker::PidCoefficients FrequencyTracker::getZcCoefficients()
{
    return m_zcController.getCoefficients();
}
// ==================================

// ==== private 함수 ====

// --- 상태 처리 함수 ---
void FrequencyTracker::processCoarseSearch(const DataPoint& latestDataPoint)
{
    // 1. 데이터 수집
    m_coarseSearchBuffer.push_back(latestDataPoint);

    // 2. 필요한 샘플이 모두 모였는지 확인
    if(m_coarseSearchBuffer.size() < m_coarseSearchSamplesNeeded) {
        return; // 아직 샘플이 더 필요함
    }

    // 3. DFT로 기본파 파형을 재구성
    std::vector<double> clean_wave = AnalysisUtils::generateFundamentalWave(m_coarseSearchBuffer);

    if(clean_wave.empty()) {
        qDebug() << "거친 탐색 실패: 기본파 못찾음. 재시작..";
        startCoarseSearch();
        return;
    }

    // 4. 재구성한 그래프로 ZC 실행
    double estimateFreq = estimateFrequencyByZeroCrossing(clean_wave);
    qDebug() << "estimate Freq : " << estimateFreq;

    if(estimateFreq < 1.0) {
        qDebug() << "거친 탐색 실패: 주파수 너무 낮음. 재시작..";
        startCoarseSearch();
        return;
    }

    // 5.  추정된 주파수로 FLL 시작
    emit samplingCyclesUpdated(estimateFreq);
    m_trackingState = TrackingState::FLL_Acquisition;
    qDebug() << " --- FLL로 전환됨 ---";

    // FLL 상태 초기화
    m_pll_previousVoltagePhase = 0.0;
}

void FrequencyTracker::processFll(const MeasuredData& latestMeasuredData)
{
    auto phaseErrorResult = calculatePhaseError(latestMeasuredData);

    if(!phaseErrorResult) { // 실패한 경우
        // 기본파를 못찾은 경우에만 실패 카운터 증가
        if(phaseErrorResult.error() == PhaseErrorType::NoFundamentalComponent) {
            if(++m_fll_failCounter >= PllConstants::MaxFailCount) {
                qDebug() << "FLL 실패 (기본파 없음). 거친 탐색 다시 시작";
                startCoarseSearch();
            }
        }

        // FirstRun의 경우, 아무것도 하지 않고 다음 샘플을 기다림
        return;
    }
    m_fll_failCounter = 0; // 성공 시 카운터 리셋

    const double phaseError = phaseErrorResult->error; // 성공값 추출
    const double cycleDuration = m_engine->parameters().samplesPerCycle * m_engine->m_captureIntervalsNs.count();
    const double frequencyError = phaseError / (config::Math::TwoPi * cycleDuration);

    // FLL 실패 감지
    if(std::abs(frequencyError) > FllConstants::FailureThresholdHz) {
        if(++m_fll_failCounter >= FllConstants::MaxFailCount) {
            qDebug() << "FLL 실패. 거친 탐색 다시 시작";
            startCoarseSearch();
            return;
        }
    } else {
        m_fll_failCounter = 0;
    }

    // PID 출력 계산
    double lf_output = m_fllController.process(frequencyError);

    // 진동 감지
    if(std::signbit(lf_output) != std::signbit(m_fll_previousLfOutput) && m_fll_previousLfOutput!= 0.0) {
        if(++m_fll_oscillationCounter >= FllConstants::MaxOscillations) {
            qDebug() << "FLL 진동 감지! 거친 탐색 다시 시작.";
            startCoarseSearch();
            return;
        }
    } else {
        m_fll_oscillationCounter = 0;
    }
    m_fll_previousLfOutput = lf_output;

    double newSamplingcycles = m_engine->parameters().samplingCycles + lf_output;
    newSamplingcycles = std::clamp(newSamplingcycles, static_cast<double>(config::Sampling::MinValue), static_cast<double>(config::Sampling::maxValue));

    if(std::abs(m_engine->parameters().samplingCycles - newSamplingcycles) > 1e-9) {
        emit samplingCyclesUpdated(newSamplingcycles);
    }

    checkFllLock(frequencyError);
}

void FrequencyTracker::processFineTune(const MeasuredData& latestMeasuredData)
{
    // 주기적인 Lock 검증 로직
    if(++m_pll_cycleCounter > VerificationConstants::IntervalCycles && !m_isVerifying) {
        startVerification();
        return;
    }

    auto phaseErrorResult = calculatePhaseError(latestMeasuredData);

    if(!phaseErrorResult) { // 실패한 경우
        if(phaseErrorResult.error() == PhaseErrorType::NoFundamentalComponent) {
            if(++m_pll_failCounter >= PllConstants::MaxFailCount) {
                qDebug() << "PLL 실패 (기본파 없음). FLL 복귀";
                m_trackingState = TrackingState::FLL_Acquisition;
                m_fllController.reset();
            }
        }
        return;
    }

    const double currentPhasorAngle = phaseErrorResult->currentAngle;
    const double phaseError = phaseErrorResult->error;

    // 실패 감지 및 재탐색
    if(std::abs(phaseError) > PllConstants::FailureThresholdRad) {
        if(++m_pll_failCounter >= PllConstants::MaxFailCount) {
            if(std::abs(phaseError) > PllConstants::SevereFailureThresholdRad) {
                qDebug() << "PLL 실패 (심각). 거친 탐색 다시 시작";
                startCoarseSearch();
            } else {
                qDebug() << "PLL 실패 (중간). FLL 복귀";
                m_trackingState = TrackingState::FLL_Acquisition;
            }
            return;
        }
    } else {
        m_pll_failCounter = 0;
    }

    // ZC Tracking (PID)
    // 목표 위상을 동적으로 결정
    const double targetPhaseMinus90 = -std::numbers::pi / 2.0;
    const double targetPhasePlus90 = std::numbers::pi / 2.0;

    // 현재 각도에서 두 목표까지의 거리 계산
    double errorMinus90 = currentPhasorAngle - targetPhaseMinus90;
    while(errorMinus90 <= -std::numbers::pi) errorMinus90 += config::Math::TwoPi;
    while(errorMinus90 > std::numbers::pi)  errorMinus90 -= config::Math::TwoPi;

    double errorPlus90 = currentPhasorAngle - targetPhasePlus90;
    while(errorPlus90 <= -std::numbers::pi) errorPlus90 += config::Math::TwoPi;
    while(errorPlus90 > std::numbers::pi) errorPlus90 -= config::Math::TwoPi;

    // 더 작은 에러를 최종 위상 에러로 선택
    double zcPhaseError = (std::abs(errorMinus90) < std::abs(errorPlus90)) ? errorMinus90 : errorPlus90;

    // PID 출력 계산
    double phase_lf_output = m_zcController.process(zcPhaseError);

    double newSamplingCycles = m_engine->parameters().samplingCycles + phase_lf_output;
    newSamplingCycles = std::clamp(newSamplingCycles, static_cast<double>(config::Sampling::MinValue), static_cast<double>(config::Sampling::maxValue));

    if(std::abs(m_engine->parameters().samplingCycles - newSamplingCycles) > 1e-9) {
        emit samplingCyclesUpdated(newSamplingCycles);
    }
    m_pll_previousVoltagePhase = phaseErrorResult->currentAngle;
}

void FrequencyTracker::processVerification(const DataPoint& latestDataPoint)
{
    // 1. 데이터 수집
    m_coarseSearchBuffer.push_back(latestDataPoint);
    if(m_coarseSearchBuffer.size() < m_coarseSearchSamplesNeeded) {
        return; // 샘플 더 필요
    }

    // 2. DFT + ZC로 주파수 재추정
    std::vector<double> clean_wave = AnalysisUtils::generateFundamentalWave(m_coarseSearchBuffer);
    if(clean_wave.empty()) {
        qDebug() << "Lock 검증 실패: DFT 분석 불가. 재시도...";
        startVerification(); // 버퍼 비우고 다시 시도
        return;
    }
    double zc_freq = estimateFrequencyByZeroCrossing(clean_wave);
    double pll_freq = m_engine->parameters().samplingCycles;

    if(zc_freq > 0) {
        double ratio = pll_freq / zc_freq;
        qDebug() << "Lock 검증: PLL Freq =" << pll_freq << ", ZC Freq =" << zc_freq << ", Ratio =" << ratio;

        // 3. 비율 확인 (0.49 ~ 1.27 범위를 정상으로 간주)
        if(ratio > VerificationConstants::ValidRatioMax || ratio < VerificationConstants::ValidRatioMin) {
            // 정수배 혹은 정수비에 Lock된 것으로 판단
            qDebug() << "잘못된 Lock 감지! 거친 탐색 다시 시작.";
            startCoarseSearch();
            return;
        }
    }

    qDebug() << "Lock 검증 통과";
    m_isVerifying = false;
    m_pll_cycleCounter = 0;
}
// -------------------

// --- 헬퍼 함수 --------
void FrequencyTracker::startCoarseSearch()
{
    resetAllStates(); // 모든 상태를 초기화하고 시작
    m_coarseSearchBuffer.clear();
    m_trackingState = TrackingState::Coarse;

    const double initialSamplingRate = 50.0 * 20.0;
    const double currentSamplingRate = m_engine->parameters().samplingCycles * m_engine->parameters().samplesPerCycle;

    // 0.5초 분량의 샘플 개수를 계산
    if(currentSamplingRate > 1.0) {
        m_coarseSearchSamplesNeeded = static_cast<int>(initialSamplingRate * CoarseSearchConstants::Duration_S);
    } else {
        // 샘플링 속도가 너무 느릴 경우 최소 샘플 개수 보장
        m_coarseSearchSamplesNeeded = CoarseSearchConstants::MinSamples;
    }
    qDebug() << "Coarse Pass 1 - Samples Needed : " << m_coarseSearchSamplesNeeded;

    // 버퍼 공간 미리 할당
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

void FrequencyTracker::resetAllStates()
{
    m_trackingState = TrackingState::Idle;

    m_coarseSearchBuffer.clear();
    m_coarseSearchSamplesNeeded = 0;
    m_isVerifying = false;

    m_fll_failCounter = 0;
    m_fll_lockCounter = 0;
    m_fll_previousLfOutput = 0.0;
    m_fll_oscillationCounter = 0;

    m_pll_previousVoltagePhase = 0.0;
    m_pll_failCounter = 0;
    m_pll_cycleCounter = 0;

    m_fllController.reset();
    m_zcController.reset();
}

double FrequencyTracker::estimateFrequencyByZeroCrossing(const std::vector<double>& wave)
{
    if(wave.size() < 2) {
        return 0.0;
    }

    int zeroCrossings = 0;
    for(size_t i{1}; i < wave.size(); ++i) {
        // 이전 샘플과 현재 샘플의 부호가 다르면 Zero-Crossing으로 간주
        if((wave[i - 1]< 0 && wave[i] >= 0) ||
            (wave[i - 1] > 0 && wave[i] <= 0))
        {
            ++zeroCrossings;
        }
    }

    // 수집된 데이터 총 시간 계산
    const auto& firstSample = m_coarseSearchBuffer.front();
    const auto& lastSample = m_coarseSearchBuffer.back();
    const double durationSeconds = std::chrono::duration_cast<utils::FpSeconds>(lastSample.timestamp - firstSample.timestamp).count();

    if(durationSeconds < 1e-6) {
        return 0.0;
    }

    // 주파수 계산: (교차 횟수 / 2) / 시간
    return (static_cast<double>(zeroCrossings) / 2.0) / durationSeconds;
}

void FrequencyTracker::checkFllLock(double frequencyError)
{
    if(std::abs(frequencyError) < FllConstants::LockThresholdHz) {
        ++m_fll_lockCounter;
    } else {
        m_fll_lockCounter = 0;
    }

    if(m_fll_lockCounter >= FllConstants::MinLockCount) {
        qDebug() << "주파수 고정됨. PLL 시작";
        m_trackingState = TrackingState::FineTune;

        // PLL을 위한 상태 초기화
        m_pll_failCounter = 0;
        m_pll_cycleCounter = 0.0;
        m_pll_previousVoltagePhase = 0.0;
        m_zcController.reset();
    }
}

void FrequencyTracker::startVerification()
{
    qDebug() << "주기적인 Lock 검증 시작...";
    m_isVerifying = true;
    m_coarseSearchBuffer.clear(); // Coarse Search 버퍼 재사용

    // 0.2초 분량의 샘플을 수집하여 검증
    const double currentSamplingRate = m_engine->parameters().samplingCycles * m_engine->parameters().samplesPerCycle;
    if(currentSamplingRate > 1.0) {
        m_coarseSearchSamplesNeeded = static_cast<int>(currentSamplingRate * VerificationConstants::Durationi_S);

    }
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

// ---------------------

std::expected<FrequencyTracker::PhaseInfo, FrequencyTracker::PhaseErrorType> FrequencyTracker::calculatePhaseError(const MeasuredData& latestMeasuredData)
{
    // 기본파 전압의 위상 정보를 가져옴
    const auto* v_fund = AnalysisUtils::getHarmonicComponent(latestMeasuredData.voltageHarmonics, 1);

    // 데이터가 없으면 넘어감
    if(!v_fund) {
        return std::unexpected(PhaseErrorType::NoFundamentalComponent);
    }

    const double currentPhasorAngle = v_fund->phase;

    // 첫 실행 시 비교할 이전 값이 없으므로 에러 처리
    if(m_pll_previousVoltagePhase == 0.0) {
        m_pll_previousVoltagePhase = currentPhasorAngle;
        return std::unexpected(PhaseErrorType::FirstRun);
    }

    // 위상차 계산 및 정규화 (-pi ~ pi)
    double phaseError = currentPhasorAngle - m_pll_previousVoltagePhase;
    while(phaseError <= -std::numbers::pi) phaseError += config::Math::TwoPi;
    while(phaseError > std::numbers::pi) phaseError -= config::Math::TwoPi;

    return PhaseInfo{currentPhasorAngle, m_pll_previousVoltagePhase, phaseError}; // 성공
}
