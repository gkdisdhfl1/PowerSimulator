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
    qDebug() << "m_captureTimer.interval()" << m_captureTimer.interval();
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
        qDebug() << "m_captureIntervalsMs: " << m_captureIntervalsMs;
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
        m_coarseSearchBuffer.clear();
        m_fineTuneFailCounter = 0;
    }
}
// -----------------------


// ---- private slots ----
void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);

    // --- 자동 추적 상태에 따른 분기 처리 ---
    switch (m_trackingState) {
    case TrackingState::Coarse:
        processCoarseSearch(); // 거친 탐색 로직 실행
        break;
    case TrackingState::Fine:
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

    // 다음 스텝을 위해 현재 진행 위상 업데이트
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
    if(m_trackingState == TrackingState::Fine)
        processFineTune();

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
    if(m_trackingState != TrackingState::Fine || m_measuredData.empty()) {
        return;
    }

    // 1. (PD) 현재 위상과 이전 위상을 비교하여 위상차 계산
    // arctan 계산
    const double currentPhasorAngle = std::atan2(m_measuredData.back().voltagePhasorY, m_measuredData.back().voltagePhasorX);

    // 첫 번째 실행인지 확인
    if(m_previousVoltagePhase == 0.0) {
        m_previousVoltagePhase = currentPhasorAngle;
        return;
    }
    // 위상차 계산
    double phaseError = currentPhasorAngle - m_previousVoltagePhase;

    // 위상 wrapping 처리 (-pi ~ pi 정규화)
    while(phaseError <= -std::numbers::pi) phaseError += 2.0 * std::numbers::pi;
    while(phaseError > std::numbers::pi) phaseError -= 2.0 * std::numbers::pi;

    //  --- 실패 감지 및 재탐색 ---
    constexpr double failureThreshold = 0.5; // 0.5 라디안 이상 벌어지면 실패로 간주 (튜닝 필요)
    constexpr int maxFailCount = 5; // 5번 연속 실패하면 재탐색

    if(std::abs(phaseError) > failureThreshold) {
        ++m_fineTuneFailCounter;
    } else {
        m_fineTuneFailCounter = 0; // 정상 범위에 들어오면 리셋
    }

    if(m_fineTuneFailCounter >= maxFailCount) {
        qDebug() << "PLL 실패. 거친 탐색 다시 시작";
        startCoarseSearch();
        return; // 현재 사이클의 PLL은 건너뜀
    }
    // ------------------------

    m_previousVoltagePhase = currentPhasorAngle;

    // 2. (LF) PI 제어기로 주파수 조정값 계산
    // Kp, Ki는 실험적으로 튜닝해야함
    constexpr double Kp = 0.85;// 비례 이득(Proportional gain)
    constexpr double Ki = 0.055; // 적분 이득 (Integral gain)
    constexpr double IntegralMax = 2.5; // 적분항 최대값 (튜닝 필요)
    constexpr double IntegralMin = -2.5; // 적분항 최소값 (튜닝 필요)

    // 적분 오차 누적
    m_integralError += phaseError;
    m_integralError = std::clamp(m_integralError, IntegralMin, IntegralMax);

    // PI 제어기에 따른 주파수 조정량 계산
    double lf_output = (Kp * phaseError) + (Ki * m_integralError);

    // PLL의 포착 범위를 위한 출력 제한
    constexpr double maxFreqChangePerStep = 0.5;
    lf_output = std::clamp(lf_output, -maxFreqChangePerStep, maxFreqChangePerStep);

    // 3. (NCO) 계산된 조정량으로 Sampling 주파수 업데이트
    double newSamplingCycles = m_params.samplingCycles + lf_output; // 위상차와 반대 방향으로 조정

    // 주파수가 비정상적인 값으로 가지 않도록 범위 제한
    newSamplingCycles = std::clamp(newSamplingCycles, (double)config::Sampling::MinValue, (double)config::Sampling::maxValue);

    if(std::abs(m_params.samplingCycles - newSamplingCycles) > 1e-6) {
        m_params.samplingCycles = newSamplingCycles;
        recalculateCaptureInterval(); //  변경된 주파수에 맞춰 샘플링 간격 재계산
        emit samplingCyclesUpdated(newSamplingCycles); // UI에 알림
    }
}

void SimulationEngine::startCoarseSearch()
{
    m_trackingState = TrackingState::Coarse;
    m_coarseSearchBuffer.clear();
    m_fineTuneFailCounter = 0;

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

        // 5. 정밀 조정  상태로 전환
        m_trackingState = TrackingState::Fine;
        m_previousVoltagePhase = 0.0; // PLL 초기화를 위해 이전 위상 리셋
        m_integralError = 0.0;
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

    const double total_time_in_buffer = N * (m_captureIntervalsMs.count() / 1000.0);
    const double k = total_time_in_buffer * m_params.samplingCycles;

    const double two_pi_k_over_N = 2.0 * std::numbers::pi * k / N;

    double squareSum = 0.0;
    double phasorX_sum = 0.0;
    double phasorY_sum = 0.0;

    for(size_t n = 0; n < N; ++n) {
        const auto& sample = m_cycleSampleBuffer[n];
        const double value = (type == DataType::Voltage) ? sample.voltage : sample.current;

        const double angle = two_pi_k_over_N * n;
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
