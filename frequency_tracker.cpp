#include "frequency_tracker.h"
#include "simulation_engine.h"
#include <QDebug>

namespace {
    struct CoarseSearchConstants {
        static constexpr double Duration_S = 0.5; // 데이터 수집 시간
        static constexpr int MinSamples = 10;
        static constexpr double MinTrackingFreq = 1.0;
        static constexpr double MaxTrackingFreqRatio = 1.8; // 에일리어싱 방지
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
    };
    struct ZcTrackerConstants {
        static constexpr double Kp = 0.015;
        static constexpr double Ki = 0.000008;
        static constexpr double Kd = 0.265;
        static constexpr double IntegralActivationThresholdRad = 0.01;
        static constexpr double Integral_Limit = 10;
    };
    struct VerificationConstants {
        static constexpr int IntervalCycles = 200;
        static constexpr double Durationi_S = 0.2;
        static constexpr double ValidRatioMin = 0.8;
        static constexpr double ValidRatioMax = 1.2;
    };
}

FrequencyTracker::FrequencyTracker(SimulationEngine* engine, QObject *parent)
    : QObject{parent}
    , m_engine(engine)
    , m_trackingState(TrackingState::Idle)
    , m_coarseSearchSamplesNeeded(0)
    , m_isVerifying(false)
    , m_fll_integralError(0.0)
    , m_fll_previousFrequencyError(0.0)
    , m_fll_failCounter(0)
    , m_fll_lockCounter(0)
    , m_fll_previousLfOutput(0.0)
    , m_fll_oscillationCounter(0)
    , m_pll_previousVoltagePhase(0.0)
    , m_pll_failCounter(0)
    , m_isFrequencyLocked(false)
    , m_pll_lockCounter(0)
    , m_pll_cycleCounter(0)
    , m_zc_integralError(0.0)
    , m_zc_previousPhaseError(0.0)
{}

// ==== public 함수 ==================
void FrequencyTracker::startTracking()
{
    resetAllStates();
    startCoarseSearch();
}

void FrequencyTracker::stopTracking()
{
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

    // 3. 샘플이 다 모였으면, Zero-Crossing으로 주파수 설정
    double estimateFreq = estimateFrequencyByZeroCrossing();

    // 4. 추정된 주파수로 파라미터 업데이트
    if(estimateFreq > 0) {
        // 에일리어싱 방지를 위해 상한선 제한
        double newSamplingCycles = std::clamp(estimateFreq, CoarseSearchConstants::MinTrackingFreq, CoarseSearchConstants::MaxTrackingFreqRatio);
        emit samplingCyclesUpdated(newSamplingCycles);

        // FLL 획득 상태로 전환
        m_trackingState = TrackingState::FLL_Acquisition;
        m_pll_previousVoltagePhase = 0.0;
        m_fll_integralError = 0.0;
    }
}

void FrequencyTracker::processFll(const MeasuredData& latestMeasuredData)
{
    const double currentPhasorAngle = std::atan2(latestMeasuredData.voltagePhasorY, latestMeasuredData.voltagePhasorX);
    if(m_pll_previousVoltagePhase == 0.0) {
        m_pll_previousVoltagePhase = currentPhasorAngle;
        return;
    }

    double phaseError = currentPhasorAngle - m_pll_previousVoltagePhase;
    while(phaseError <= -std::numbers::pi) phaseError += 2.0 * std::numbers::pi;
    while(phaseError > std::numbers::pi) phaseError -= 2.0 * std::numbers::pi;

    // FLL에서 주파수 에러를 직접 제어
    const double cycleDuration = m_engine->parameters().samplesPerCycle * (m_engine->m_captureIntervalsMs.count() / 1000.0);
    const double frequencyError = phaseError / (2.0 * std::numbers::pi * cycleDuration);

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

    // I항 누적
    m_fll_integralError += frequencyError;
    m_fll_integralError = std::clamp(m_fll_integralError, -FllConstants::IntegralLimit, FllConstants::IntegralLimit);

    // D항 누적
    double derivative = frequencyError - m_fll_previousFrequencyError;

    // PID 출력 계산
    double lf_output = (FllConstants::Kp * frequencyError) + (FllConstants::Ki * m_fll_integralError) + (FllConstants::Kd * derivative);

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

    m_fll_previousFrequencyError = frequencyError;
    lf_output = std::clamp(lf_output, -FllConstants::OutputLimit, FllConstants::OutputLimit);

    double newSamplingcycles = m_engine->parameters().samplingCycles + lf_output;
    newSamplingcycles = std::clamp(newSamplingcycles, static_cast<double>(config::Sampling::MinValue), static_cast<double>(config::Sampling::maxValue));

    if(std::abs(m_engine->parameters().samplingCycles - newSamplingcycles) > 1e-9) {
        emit samplingCyclesUpdated(newSamplingcycles);
    }

    checkFllLock(frequencyError);
    m_pll_previousVoltagePhase = currentPhasorAngle;
}

void FrequencyTracker::processFineTune(const MeasuredData& latestMeasuredData)
{
    if(++m_pll_cycleCounter > VerificationConstants::IntervalCycles && !m_isVerifying) {
        startVerification();
        return;
    }

    // (PD) 현재 위상과 이전 위상을 비교하여 위상차 계산
    // arctan 계산
    const double currentPhasorAngle = std::atan2(latestMeasuredData.voltagePhasorY, latestMeasuredData.voltagePhasorX);

    // 첫 번째 실행인지 확인
    if(m_pll_previousVoltagePhase == 0.0) {
        m_pll_previousVoltagePhase = currentPhasorAngle;
        return;
    }

    // 위상차 계산 및 정규화 (-pi ~ pi)
    double phaseError = currentPhasorAngle - m_pll_previousVoltagePhase;
    while(phaseError <= -std::numbers::pi) phaseError += 2.0 * std::numbers::pi;
    while(phaseError > std::numbers::pi) phaseError -= 2.0 * std::numbers::pi;

    // 실패 감지 및 재탐색
    if(std::abs(phaseError) > PllConstants::FailureThresholdRad) {
        if(++m_pll_failCounter >= PllConstants::MaxFailCount) {
            if(std::abs(phaseError) > PllConstants::SevereFailureThresholdRad) {
                qDebug() << "PLL 실패 (심각). 거친 탐색 다시 시작";
                startCoarseSearch();
            } else {
                qDebug() << "PLL 실패 (중간). FLL 복귀";
                m_trackingState = TrackingState::FLL_Acquisition;
                m_fll_integralError = 0.0;
            }
            return;
        }
    } else {
        m_pll_failCounter = 0;
    }
    m_pll_previousVoltagePhase = currentPhasorAngle;

    // ZC Tracking (PID)
    // 목표 위상을 동적으로 결정
    const double targetPhaseMinus90 = -std::numbers::pi / 2.0;
    const double targetPhasePlus90 = std::numbers::pi / 2.0;

    // 현재 각도에서 두 목표까지의 거리 계산
    double errorMinus90 = currentPhasorAngle - targetPhaseMinus90;
    while(errorMinus90 <= -std::numbers::pi) errorMinus90 += 2.0 * std::numbers::pi;
    while(errorMinus90 > std::numbers::pi)  errorMinus90 -= 2.0 * std::numbers::pi;

    double errorPlus90 = currentPhasorAngle - targetPhasePlus90;
    while(errorPlus90 <= -std::numbers::pi) errorPlus90 += 2.0 * std::numbers::pi;
    while(errorPlus90 > std::numbers::pi) errorPlus90 -= 2.0 * std::numbers::pi;

    // 더 작은 에러를 최종 위상 에러로 선택
    double zcPhaseError = (std::abs(errorMinus90) < std::abs(errorPlus90)) ? errorMinus90 : errorPlus90;

    double derivative = zcPhaseError - m_zc_previousPhaseError;

    if(std::abs(zcPhaseError) < ZcTrackerConstants::IntegralActivationThresholdRad)     {
        m_zc_integralError += zcPhaseError;
    } else {
        m_zc_integralError = 0.0;
    }
    m_zc_integralError = std::clamp(m_zc_integralError, -ZcTrackerConstants::Integral_Limit, ZcTrackerConstants::Integral_Limit);

    double phase_lf_output = (ZcTrackerConstants::Kp * zcPhaseError) + (ZcTrackerConstants::Ki * m_zc_integralError) + (ZcTrackerConstants::Kd * derivative);
    m_zc_previousPhaseError = zcPhaseError;

    double newSamplingCycles = m_engine->parameters().samplingCycles + phase_lf_output;
    newSamplingCycles = std::clamp(newSamplingCycles, static_cast<double>(config::Sampling::MinValue), static_cast<double>(config::Sampling::maxValue));
    if(std::abs(m_engine->parameters().samplingCycles - newSamplingCycles) > 1e-9) {
        emit samplingCyclesUpdated(newSamplingCycles);
    }
}

void FrequencyTracker::processVerification(const DataPoint& latestDataPoint)
{
    // 1. 데이터 수집
    m_coarseSearchBuffer.push_back(latestDataPoint);
    if(m_coarseSearchBuffer.size() < m_coarseSearchSamplesNeeded) {
        return; // 샘플 더 필요
    }

    // 2. ZC로 주파수 재추정
    double zc_freq = estimateFrequencyByZeroCrossing();
    double pll_freq = m_engine->parameters().samplingCycles;

    if(zc_freq > 0) {
        double ratio = pll_freq / zc_freq;
        qDebug() << "Lock 검증: PLL Freq =" << pll_freq << ", ZC Freq =" << zc_freq << ", Ratio =" << ratio;

        // 3. 비율 확인 (0.8 ~ 1.2 범위를 정상으로 간주)
        if(ratio > 1.2 || ratio < 0.8) {
            // 정수배 혹은 정수비에 Lock된 것으로 판단
            qDebug() << "잘못된 고조파 Lock 감지! 거친 탐색 다시 시작.";
            startCoarseSearch();
            return;
        }
    }

    qDebug() << "잘못된 고조파 Lock 감지! 거친 탐색 다시 시작.";
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

    // 0.5초 분량의 샘플 개수를 계산
    const double currentSamplingRate = m_engine->parameters().samplingCycles * m_engine->parameters().samplesPerCycle;
    if(currentSamplingRate > 1.0) {
        m_coarseSearchSamplesNeeded = static_cast<int>(currentSamplingRate * CoarseSearchConstants::Duration_S);
    } else {
        // 샘플링 속도가 너무 느릴 경우 최소 샘플 개수 보장
        m_coarseSearchSamplesNeeded = CoarseSearchConstants::MinSamples;
    }

    // 버퍼 공간 미리 할당
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

void FrequencyTracker::resetAllStates()
{
    m_trackingState = TrackingState::Idle;

    m_coarseSearchBuffer.clear();
    m_coarseSearchSamplesNeeded = 0;
    m_isVerifying = false;

    m_fll_integralError = 0.0;
    m_fll_previousFrequencyError = 0.0;
    m_fll_failCounter = 0;
    m_fll_lockCounter = 0;
    m_fll_previousLfOutput = 0.0;
    m_fll_oscillationCounter = 0;

    m_pll_previousVoltagePhase = 0.0;
    m_pll_failCounter = 0;
    m_pll_lockCounter = 0;
    m_pll_cycleCounter = 0;
    m_isFrequencyLocked = false;

    m_zc_integralError = 0.0;
    m_zc_previousPhaseError = 0.0;
}

double FrequencyTracker::estimateFrequencyByZeroCrossing()
{
    if(m_coarseSearchBuffer.size() > 2) {
        return 0.0;
    }

    int zeroCrossings = 0;
    for(size_t i{1}; i < m_coarseSearchBuffer.size(); ++i) {
        // 이전 샘플가 환져 샘플의 부호가 다르면 Zero-Crossing으로 간주
        if((m_coarseSearchBuffer[i - 1].voltage < 0 && m_coarseSearchBuffer[i].voltage >= 0) ||
            (m_coarseSearchBuffer[i - 1].voltage > 0 && m_coarseSearchBuffer[i].voltage <= 0))
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
        m_zc_integralError = 0.0;
        m_zc_previousPhaseError = 0.0;
        m_pll_cycleCounter = 0.0;
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
    } else {
        m_coarseSearchSamplesNeeded = CoarseSearchConstants::MinSamples;
    }
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

// ---------------------
