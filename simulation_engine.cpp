     #include "simulation_engine.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_accumulatedPhaseSinceUpdate(0.0)
    , m_captureIntervalsMs(0.0)
    , m_simulationTimeNs(0)
    , m_previousVoltagePhase(0.0)
    , m_integralError(0.0)
    , m_trackingState(TrackingState::Idle)
    , m_coarseSearchSamplesNeeded(0)
    , m_fineTuneFailCounter(0)
    , m_isFrequencyLocked(false)
    , m_frequencyLockCounter(0)
    , m_phaseIntegralError(0.0)
    , m_previousZcPhaseError(0.0)
    , m_fineTuneCycleCounter(0)
    , m_fllFailCounter(0)
    , m_isVerifying(false)
    , m_previousFrequencyError(0.0)
{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsMs = 1.0s / (m_params.samplingCycles * m_params.samplesPerCycle);

    connect(&m_captureTimer, &QTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정
}

// ---- public -----
bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
int SimulationEngine::getDataSize() const { return m_data.size(); }
SimulationEngine::Parameters& SimulationEngine::parameters() { return m_params; };
const SimulationEngine::Parameters& SimulationEngine::parameters() const { return m_params; };
// -----------------

// ---- public slots ----
void SimulationEngine::start()
{
    if (isRunning()) return;
    qDebug() << "시작";
    qDebug() << "m_amplitude = " << m_params.amplitude << " " << "m_frequency = " << m_params.frequency;
    qDebug() << "m_maxDataSize = " << m_params.maxDataSize << "m_timeScale = " << m_params.timeScale;

    m_captureTimer.start();
    emit runningStateChanged(true);
    qDebug() << "Engine started.";
}

void SimulationEngine::stop()
{
    if (!isRunning()) return;

    m_captureTimer.stop();

    emit runningStateChanged(false);
    qDebug() << "Engine stopped.";
}

void SimulationEngine::onMaxDataSizeChanged(int newSize)
{
    m_params.maxDataSize = newSize;

    while(m_data.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_data.pop_front();
    }
    while(m_measuredData.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_measuredData.pop_front();
    }

    emit dataUpdated(m_data);
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    double scaledIntervalMs = (m_captureIntervalsMs * m_params.timeScale).count();

    // 간격이 0이 되는 것을 방지
    if(scaledIntervalMs < config::Simulation::Timer::MinIntervalMs)
        scaledIntervalMs = config::Simulation::Timer::MinIntervalMs;

    // int 최대값 초과 방지
    const double maxTimerInterval = static_cast<double>(std::numeric_limits<int>::max());
    if(scaledIntervalMs > maxTimerInterval)
        scaledIntervalMs = maxTimerInterval;

    m_captureTimer.setInterval(static_cast<int>(std::round(scaledIntervalMs)));
    // qDebug() << "m_captureTimer.interval()" << m_captureTimer.interval();
}

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_params.samplingCycles * m_params.samplesPerCycle;
    // qDebug() << "---------------------------------------";
    // qDebug() << "SimulationEngine::recalculateCaptureInterval()";
    // qDebug() << "---------------------------------------";/*
    // qDebug() << "m_params.samplingCycles : " << m_params.samplingCycles;
    // qDebug() << "m_params.samplesPerCycle : " << m_params.samplesPerCycle;
    // qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;
    // qDebug() << "---------------------------------------";

    if(totalSamplesPerSecond > config::Sampling::MaxSamplesPerSecond) {
        totalSamplesPerSecond = config::Sampling::MaxSamplesPerSecond;
        qWarning() << "Sampling rate 이 너무 높음. 최대값으로 조정됨.";
    }
    if(totalSamplesPerSecond > 0) {
        m_captureIntervalsMs = 1.0s / totalSamplesPerSecond;
        // qDebug() << "m_captureIntervalsMs: " << m_captureIntervalsMs;
    } else {
        m_captureIntervalsMs = FpMilliseconds(1.0e9);
    }

    updateCaptureTimer();
}

void SimulationEngine::onRedrawRequest()
{
    // 시뮬레이션이 멈춰있을 때만 전체 데이터 강제 업데이트
    // (예: 자동 스크롤이 꺼진 상태에서 그래프와 상호작용할 때)
    if(!isRunning()) {
        emit dataUpdated(m_data);
        return;
    }

    // 누적된 위상을 보고 Mode에 맞춰 update 요청
    processUpdateByMode(false); // 누적 위상 리셋 안함
}

void SimulationEngine::onRedrawAnalysisRequest()
{
    emit measuredDataUpdated(m_measuredData);
}

void SimulationEngine::enableFrequencyTracking(bool enabled)
{
    if(enabled) {
        // 추적 시작
        startCoarseSearch();
    } else {
        // 추적 중지
        m_trackingState = TrackingState::Idle;
        m_integralError = 0.0;
        m_phaseIntegralError = 0.0;
        m_coarseSearchBuffer.clear();
        m_fineTuneFailCounter = 0;
        m_isFrequencyLocked = false;
        m_frequencyLockCounter = 0.0;
        m_previousZcPhaseError = 0.0;
        m_fineTuneCycleCounter = 0;
        m_fllFailCounter = 0;
        m_isVerifying = false;
        m_previousFrequencyError = 0.0;
    }
}
// -----------------------


// ---- private slots ----
void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);

    // 백그라운드 검증 데이터 수집
    if(m_isVerifying)
        processVerification();

    // --- 자동 추적 상태에 따른 분기 처리 ---
    switch (m_trackingState) {
    case TrackingState::Coarse:
        processCoarseSearch(); // 거친 탐색 로직 실행
        break;
    case TrackingState::FLL_Acquisition:
        // FLL은 매 사이클 데이터가 필요
        if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle)) {
            calculateCycleData(); // 내부에서 processFll 호출
        }
        break;
    case TrackingState::FineTune:
        // 정밀 조정 상태일 때만 사이클 데이터 계산
        // 버퍼에 설정된 samplesPerCycle 만큼 데이터가 쌓이면 계산을 실행
        if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle)) {
            calculateCycleData();
        }
        break;
    case TrackingState::Idle:
        if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle)) {
            calculateCycleData();
        }
        break;
    default:
        break;
    }
    // -----------------------------------

    // 사이클 계산을 위해 버퍼 채우기
    m_cycleSampleBuffer.push_back(m_data.back());
    if(m_cycleSampleBuffer.size() > static_cast<size_t>(m_params.samplesPerCycle)) {
        m_cycleSampleBuffer.erase(m_cycleSampleBuffer.begin());
    }

    // 다음 스텝을 위해 현재 진행 위상 업데이트`
    const FpSeconds timeDelta = m_captureIntervalsMs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_params.frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, 2.0 * std::numbers::pi);

    // UI 갱신 및 사이클 계산을 위한 누적 위상 업데이트
    m_accumulatedPhaseSinceUpdate += phaseDelta;

    // 누적된 위상을 보고 Mode에 맞춰 업데이트
    processUpdateByMode(true); // 누적 위상 리셋
    advanceSimulationTime();
}
// -----------------------


// ---- private 함수들 ----
void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsMs);
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage() const
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians;
    return m_params.amplitude * sin(finalPhase);
}

double SimulationEngine::calculateCurrentAmperage() const
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians + m_params.currentPhaseOffsetRadians;
    return m_params.currentAmplitude * sin(finalPhase);
}

void SimulationEngine::addNewDataPoint(double voltage, double current)
{
    // DataPoint 객체를 생성하여 저장
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    // qDebug() << "voltage: " << voltage;
    // qDebug() << "current: " << current;
    m_data.push_back({m_simulationTimeNs, voltage, current});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_params.maxDataSize)) {
        // qDebug() << " ---- data{" << m_simulationTimeNs << ", " << voltage << "} 삭제 ----";
        m_data.pop_front();
    }

}

void SimulationEngine::calculateCycleData()
{
    if(m_cycleSampleBuffer.empty())
        return;

    // 1. 헬퍼 함수를 이용해 전압/전류의 핵심 지표들을 계산
    CycleMetrics voltageMetrics = calculateMetricsFor(DataType::Voltage);
    CycleMetrics currentMetrics = calculateMetricsFor(DataType::Current);

    // 2. 유효 전력 계산
    double powerSum = 0.0;
    for(const auto& sample : m_cycleSampleBuffer) {
        powerSum += sample.voltage * sample.current;
    }
    const double activePower = powerSum / m_cycleSampleBuffer.size();

    // 3. 계산된 데이터 구조체를 담아 컨테이너 추가
    m_measuredData.push_back({
        m_simulationTimeNs, // 현재 시간 (사이클 종료 시점)
        voltageMetrics.rms,
        currentMetrics.rms,
        activePower,
        voltageMetrics.phasorX,
        voltageMetrics.phasorY,
        currentMetrics.phasorX,
        currentMetrics.phasorY,
    });

    if(m_measuredData.size() > static_cast<size_t>(m_params.maxDataSize)) {
        m_measuredData.pop_front();
    }

    // 4. UI에 업데이트 알림
    emit measuredDataUpdated(m_measuredData);

    // 5. 자동 주파수 추적 로직 실행
    if(m_trackingState == TrackingState::FLL_Acquisition) {
        // FLL은 위상차만 필요로함
        const double currentPhasorAngle = std::atan2(voltageMetrics.phasorY, voltageMetrics.phasorX);
        double phaseError = currentPhasorAngle - m_previousVoltagePhase;
        while(phaseError <= -std::numbers::pi) phaseError += 2.0 * std::numbers::pi;
        while(phaseError > std::numbers::pi) phaseError -= 2.0 * std::numbers::pi;

        processFll(phaseError);
        m_previousVoltagePhase = currentPhasorAngle;
    } else if(m_trackingState == TrackingState::FineTune) {
        processFineTune();
    }

    // 6. 버퍼 비우기
    m_cycleSampleBuffer.clear();
}

void SimulationEngine::processUpdateByMode(bool resetAccumulatedPhase)
{
    bool shouldEmitUpdate = false;
    switch (m_params.updateMode) {
    case UpdateMode::PerSample:
        shouldEmitUpdate = true;
        break;
    case UpdateMode::PerHalfCycle:
        // 누적된 위상이 PI(180도) 이상 변했는지 확인
        if(m_accumulatedPhaseSinceUpdate >= std::numbers::pi) {
            shouldEmitUpdate = true;
        }
        break;
    case UpdateMode::PerCycle:
        // 누적된 위상이 2*PI 이상 변했는지 확인
        if(m_accumulatedPhaseSinceUpdate >= 2.0 * std::numbers::pi) {
            shouldEmitUpdate = true;
        }
        break;
    }
    if(shouldEmitUpdate) {
        emit dataUpdated(m_data);
        if(resetAccumulatedPhase) {
            if(m_params.updateMode == UpdateMode::PerHalfCycle)
                m_accumulatedPhaseSinceUpdate -= std::numbers::pi;
            else if(m_params.updateMode == UpdateMode::PerCycle)
                m_accumulatedPhaseSinceUpdate -= 2.0 * std::numbers::pi;
            else
                m_accumulatedPhaseSinceUpdate = 0.0;
        }
    }
}

void SimulationEngine::processFineTune()
{
    if(m_trackingState != TrackingState::FineTune || m_measuredData.empty()) {
        return;
    }

    // 주기적인 검증
    ++m_fineTuneCycleCounter;
    constexpr int verificationCycleInterval = 200; // 200 사이클마다 검증
    if(m_fineTuneCycleCounter > verificationCycleInterval) {
        if(!m_isVerifying) { // 이미 검증 중이 아닐 때만 시작
            startVerification();
        }
        return;
    }
    qDebug() << "m_fineTuneCycleCounter: " << m_fineTuneCycleCounter;

    // (PD) 현재 위상과 이전 위상을 비교하여 위상차 계산
    // arctan 계산
    const double currentPhasorAngle = std::atan2(m_measuredData.back().voltagePhasorY, m_measuredData.back().voltagePhasorX);

    // 첫 번째 실행인지 확인
    if(m_previousVoltagePhase == 0.0) {
        m_previousVoltagePhase = currentPhasorAngle;
        return;
    }

    // 위상차 계산
    double phaseError = currentPhasorAngle - m_previousVoltagePhase;
    // qDebug() << "abs(phaseError) : " << std::abs(phaseError);

    // 위상 wrapping 처리 (-pi ~ pi 정규화)
    while(phaseError <= -std::numbers::pi) phaseError += 2.0 * std::numbers::pi;
    while(phaseError > std::numbers::pi) phaseError -= 2.0 * std::numbers::pi;

    //  --- 실패 감지 및 재탐색 ---
    constexpr double coarseSearchThreshold = 0.6; //  심각한 실패
    constexpr double fllFallbackThreshold = 0.3; // 중간 실패
    constexpr int maxFailCount = 5; // 5번 연속 실패하면 재탐색

    if(std::abs(phaseError) > fllFallbackThreshold) {
        ++m_fineTuneFailCounter;
    } else {
        m_fineTuneFailCounter = 0; // 정상 범위에 들어오면 리셋
    }

    if(m_fineTuneFailCounter >= maxFailCount) {
        // 심각성 판단
        if(std::abs(phaseError) > coarseSearchThreshold) {
            qDebug() << "PLL 실패 (심각). 거친 탐색 다시 시작";
            startCoarseSearch();
        } else {
            qDebug() << "PLL 실패 (중간). FLL 복귀";
            m_trackingState = TrackingState::FLL_Acquisition;
            m_integralError = 0.0;
        }
        return; // 현재 사이클의 PLL은 건너뜀
    }

    m_previousVoltagePhase = currentPhasorAngle;
    //  ------------------------


    // 1. 목표 위상을 동적으로 결정
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
    qDebug() << "errorMinus90 : " << errorMinus90;
    qDebug() << "errorPlus90 : " << errorPlus90;
    qDebug() << "current zcPhaseError : " << zcPhaseError;

    // 2. 위상 추적용 PID 제어기
    constexpr double zcKp = 0.015;
    constexpr double zcKd = 0.265;
    constexpr double zcKi = 0.000008;

    double derivative = zcPhaseError - m_previousZcPhaseError;

    constexpr double integration_threshold = 0.01;
    if(std::abs(zcPhaseError) < integration_threshold) {
        m_phaseIntegralError += zcPhaseError;
    } else {
        m_phaseIntegralError = 0.0;
    }

    m_phaseIntegralError = std::clamp(m_phaseIntegralError, -1.0, 1.0);

    double phase_lf_output = (zcKp * zcPhaseError) + (zcKi * m_phaseIntegralError) + (zcKd * derivative);

    m_previousZcPhaseError = zcPhaseError;

    double newSamplingCycles = m_params.samplingCycles + phase_lf_output;
    qDebug() << "newSamplingCycles = " << newSamplingCycles;
    qDebug() << "-------------------------------------------";

    newSamplingCycles = std::clamp(newSamplingCycles, static_cast<double>(config::Sampling::MinValue), static_cast<double>(config::Sampling::maxValue));
    if(std::abs(m_params.samplingCycles - newSamplingCycles) > 1e-9) {
        m_params.samplingCycles = newSamplingCycles;
        recalculateCaptureInterval();
        emit samplingCyclesUpdated(newSamplingCycles);
    }
}

void SimulationEngine::startCoarseSearch()
{
    m_trackingState = TrackingState::Coarse;
    m_coarseSearchBuffer.clear();
    m_fineTuneFailCounter = 0;
    m_isFrequencyLocked = false;
    m_frequencyLockCounter = 0;
    m_phaseIntegralError = 0.0;
    m_fineTuneCycleCounter = 0;
    m_fllFailCounter = 0;
    m_isVerifying = false;
    m_previousFrequencyError = 0.0;

    // 0.5초의 분량의 샘플 개수를 계산
    const double currentSamplingRate = m_params.samplingCycles * m_params.samplesPerCycle;
    if(currentSamplingRate > 1.0) {
        m_coarseSearchSamplesNeeded = static_cast<int>(currentSamplingRate * 0.5);
    } else {
        // 샘플링 속도가 너무 느릴 경우 최소 샘플 개수 보장
        m_coarseSearchSamplesNeeded = 10;
    }

    // 버퍼 공간 미리 할당
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

void SimulationEngine::processCoarseSearch()
{
    // 1. 데이터 수집
    m_coarseSearchBuffer.push_back(m_data.back());

    // 2. 필요한 샘플이 모두 모였는지 확인
    if(m_coarseSearchBuffer.size() < m_coarseSearchSamplesNeeded) {
        return; // 아직 샘플이 더 필요함
    }

    // 3. 샘플이 다 모였으면, Zero-Crossing으로 주파수 추정
    double estimatedFreq = estimateFrequencyByZeroCrossing();

    // 4. 추정된 주파수로 파라미터 업데이트
    if(estimatedFreq > 0) {
        // 에일리어싱 방지를 위해 상한선 제한
        const double maxTrackingFrequency = estimatedFreq * 1.8;
        const double minTrackingFrequency = config::Sampling::MinValue;

        m_params.samplingCycles = std::clamp(estimatedFreq, minTrackingFrequency, maxTrackingFrequency);

        recalculateCaptureInterval();
        emit samplingCyclesUpdated(m_params.samplingCycles);

        // 5. FLL 획득 상태로 전환
        m_trackingState = TrackingState::FLL_Acquisition;
        m_previousVoltagePhase = 0.0; // 이전 위상 리셋
        m_integralError = 0.0;
    }
}

void SimulationEngine::checkFrequencyLock(double phaseError)
{
    // 주파수 고정 감지
    constexpr double lockThreshold = 0.005; // 0.005라디안 이내면 안정된 것으로 간주
    constexpr int minLockCount = 10; // 10번 연속 안정적이면 고정된 것으로 판단
    if(std::abs(phaseError) < lockThreshold) {
        ++m_frequencyLockCounter;
    } else {
        m_frequencyLockCounter = 0;
    }

    if(m_frequencyLockCounter >= minLockCount) {
        qDebug() << "주파수 고정됨. 위상 추적중..";
        m_isFrequencyLocked = true;
    }
}

double SimulationEngine::estimateFrequencyByZeroCrossing()
{
    if(m_coarseSearchBuffer.size() < 2) {
        return 0.0;
    }

    int zeroCrossings = 0;
    for(size_t i = 1; i < m_coarseSearchBuffer.size(); ++i) {
        // 이전 샘플과 현재 샘플의 부호가 다르면 Zero-Crossing으로 간주
        if((m_coarseSearchBuffer[i - 1].voltage < 0 && m_coarseSearchBuffer[i].voltage >= 0) ||
             (m_coarseSearchBuffer[i - 1].voltage > 0 && m_coarseSearchBuffer[i].voltage <= 0))
        {
            zeroCrossings++;
        }
    }

    // 수집된 데이터의 총 시간 계산
    const auto& firstSample = m_coarseSearchBuffer.front();
    const auto& lastSample = m_coarseSearchBuffer.back();
    const double durationSeconds = std::chrono::duration_cast<FpSeconds>(lastSample.timestamp - firstSample.timestamp).count();

    if(durationSeconds < 1e-6) {
        return 0.0;
    }

    // 주파수 계산: (교차 횟수 / 2) / 시간
    return (static_cast<double>(zeroCrossings) / 2.0) / durationSeconds;
}

SimulationEngine::CycleMetrics SimulationEngine::calculateMetricsFor(DataType type) const
{
    if(m_cycleSampleBuffer.empty()) {
        return {0.0, 0.0, 0.0};
    }

    // // -- 디버그 코드 ---
    // if(type == DataType::Voltage) { // 전압 계산 시 한 번만 출력
    //     qDebug() << "--- Cycle Buffer (Voltage) ---";
    //     for(size_t i = 0; i < m_cycleSampleBuffer.size(); ++i) {
    //         qDebug() << i << ":" << m_cycleSampleBuffer[i].voltage;
    //     }
    //     qDebug() << "------------------------------";
    // }
    // // ----------------

    const size_t N = m_cycleSampleBuffer.size();

    const double two_pi_over_N = 2.0 * std::numbers::pi / N;

    double squareSum = 0.0;
    double phasorX_sum = 0.0;
    double phasorY_sum = 0.0;

    for(size_t n = 0; n < N; ++n) {
        const auto& sample = m_cycleSampleBuffer[n];
        const double value = (type == DataType::Voltage) ? sample.voltage : sample.current;

        const double angle = two_pi_over_N * n;
        const double cos_angle = std::cos(angle);
        const double sin_angle = std::sin(angle);

        squareSum += value * value;
        phasorX_sum += value * cos_angle;
        phasorY_sum -= value * sin_angle; // 허수부는 -sin을 곱함
    }

    // DFT 정규화
    const double normFactor = 2.0 / N;
    return {
        .rms = std::sqrt(squareSum / N),
        .phasorX = normFactor * phasorX_sum,
        .phasorY = normFactor * phasorY_sum
    };
}

void SimulationEngine::processFll(double phaseError)
{
    // FLL은 주파수 에러를 직접 제어
    const double cycleDuration = m_params.samplesPerCycle * (m_captureIntervalsMs.count() / 1000.0);
    const double frequencyError = phaseError / (2.0 * std::numbers::pi * cycleDuration);
    qDebug() << "current frequencyError = " << frequencyError;

    // --- FLL 실패 감지 로직 ---
    constexpr double fllFailureThreshold = 10.0; // 10Hz 이상 에러가 감지되면 FLL의 처리 범위를 벗어난 것으로 간주
    constexpr int maxFllFailCount = 10;
    if(std::abs(frequencyError) > fllFailureThreshold) {
        ++m_fllFailCounter;
    } else {
        m_fllFailCounter = 0;
    }

    if(m_fllFailCounter >= maxFllFailCount) {
        qDebug() << "FLL 실패. 거친 탐색 다시 시작";
        startCoarseSearch();
        return; // 현재 FLL 중단
    }
    // ------------------------

    // FLL용 PI 제어기
    constexpr double fll_kp = 0.40;
    constexpr double fll_ki = 0.025;
    constexpr double fll_kd = 0.65;

    // D항 계산
    double derivative = frequencyError - m_previousFrequencyError;

    // I항 누적
    m_integralError += frequencyError;
    m_integralError = std::clamp(m_integralError, -10.0, 10.0);

    // PID 출력 계산
    double lf_output = (fll_kp * frequencyError) + (fll_ki * m_integralError) + (fll_kd * derivative);
    m_previousFrequencyError = frequencyError;

    lf_output = std::clamp(lf_output, -1.0, 1.0);

    double newSamplingCycles = m_params.samplingCycles + lf_output;
    qDebug() << "newSamplingCycles = " << newSamplingCycles;
    qDebug() << "-------------------------------------------";
    newSamplingCycles = std::clamp(newSamplingCycles, (double)config::Sampling::MinValue, (double)config::Sampling::maxValue);

    if(std::abs(m_params.samplingCycles - newSamplingCycles) > 1e-6) {
        m_params.samplingCycles = newSamplingCycles;
        recalculateCaptureInterval();
        emit samplingCyclesUpdated(newSamplingCycles);
    }

    checkFllLock(frequencyError);
}

void SimulationEngine::checkFllLock(double frequencyError)
{
    constexpr double lockThreshold_hz = 0.05; // 0.1Hz 이내면 Lock 간주
    constexpr int minLockCount = 10;

    if(std::abs(frequencyError) < lockThreshold_hz) {
        m_frequencyLockCounter++;
    } else {
        m_frequencyLockCounter = 0;
    }

    if (m_frequencyLockCounter >= minLockCount) {
        qDebug() << "주파수 고정됨. PLL 시작";
        m_trackingState = TrackingState::FineTune;
        // PLL을 위한 상태 초기화
        m_integralError = 0.0;
        m_phaseIntegralError = 0.0;
        m_previousZcPhaseError = 0.0;
        m_fineTuneCycleCounter = 0;
        m_previousFrequencyError = 0.0;
    }
}

void SimulationEngine::startVerification()
{
    qDebug() << "주기적인 Lock 검증 시작...";
    m_isVerifying = true;
    m_coarseSearchBuffer.clear(); //Coarse Search 버퍼 재사용

    // 0.2초 분량의 샘플을 수집하여 검증
    const double currentSamplingRate = m_params.samplingCycles * m_params.samplesPerCycle;
    if(currentSamplingRate > 1.0) {
        m_coarseSearchSamplesNeeded = static_cast<int>(currentSamplingRate * 0.2);
    } else {
        m_coarseSearchSamplesNeeded = 10;
    }
    m_coarseSearchBuffer.reserve(m_coarseSearchSamplesNeeded);
}

void SimulationEngine::processVerification()
{
    // 1. 데이터 수집
    m_coarseSearchBuffer.push_back(m_data.back());
    if(m_coarseSearchBuffer.size() < m_coarseSearchSamplesNeeded) {
        return; // 샘플 더 필요
    }

    // 2. ZC로 주파수 재추정
    double zc_freq = estimateFrequencyByZeroCrossing();
    double pll_freq = m_params.samplingCycles;

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

    // 4. 검증 통과 시, 다시 FineTune 상태로 복귀
    qDebug() << "Lock 검증 통과. Fine-tuning 계속...";
    m_isVerifying = false;
    m_fineTuneCycleCounter = 0; // 다음 검증을 위해 카운터 리셋

    // // pll 상태 초기화
    // m_previousVoltagePhase = 0.0;
    // m_phaseIntegralError = 0.0;
    // m_previousZcPhaseError = 0.0;
}
