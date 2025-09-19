#include "simulation_engine.h"
#include "frequency_tracker.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_captureIntervalsNs(0)
    , m_simulationTimeNs(0)
    , m_sampleCounterForUpdate(0)
{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsNs = 1.0s / (m_params.samplingCycles * m_params.samplesPerCycle);

    connect(&m_captureTimer, &QChronoTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정

    // FrequencyTracker 생성 및 시그널 연결
    m_frequencyTracker = std::make_unique<FrequencyTracker>(this, this);
    connect(m_frequencyTracker.get(), &FrequencyTracker::samplingCyclesUpdated, this, [this](double newFreq) {
        m_params.samplingCycles = newFreq;
        recalculateCaptureInterval();
        emit samplingCyclesUpdated(newFreq);
    });
}

// ---- public -----
bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
int SimulationEngine::getDataSize() const { return m_data.size(); }
SimulationEngine::Parameters& SimulationEngine::parameters() { return m_params; };
const SimulationEngine::Parameters& SimulationEngine::parameters() const { return m_params; };
FrequencyTracker* SimulationEngine::getFrequencyTracker() const { return m_frequencyTracker.get(); }
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
    const auto scaledIntervalNs = m_captureIntervalsNs * m_params.timeScale;

    // QChornoTimer는 std::chrono::duration을 직접 인자로 받음
    m_captureTimer.setInterval(std::chrono::duration_cast<Nanoseconds>(scaledIntervalNs));

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

    // if(totalSamplesPerSecond > config::Sampling::MaxSamplesPerSecond) {
    //     totalSamplesPerSecond = config::Sampling::MaxSamplesPerSecond;
    //     qWarning() << "Sampling rate 이 너무 높음. 최대값으로 조정됨.";
    // }
    if(totalSamplesPerSecond > 0) {
        m_captureIntervalsNs = 1.0s / totalSamplesPerSecond;
        // qDebug() << "m_captureIntervalsMs: " << m_captureIntervalsNs / 1000000;
        // qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;
    } else {
        m_captureIntervalsNs = FpNanoseconds(1.0e9);
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
        m_frequencyTracker->startTracking();
    } else {
        m_frequencyTracker->stopTracking();
    }
}
// -----------------------


// ---- private slots ----
void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);


    // 사이클 계산을 위해 버퍼 채우기
    m_cycleSampleBuffer.push_back(m_data.back());
    if(m_cycleSampleBuffer.size() > static_cast<size_t>(m_params.samplesPerCycle)) {
        m_cycleSampleBuffer.erase(m_cycleSampleBuffer.begin());
    }

    m_frequencyTracker->process(m_data.back(), m_measuredData.empty() ? MeasuredData{} : m_measuredData.back(), m_cycleSampleBuffer);

    // 사이클이 꽉 찼으면 사이클 단위 연산 수행
    if(m_cycleSampleBuffer.size() >= static_cast<size_t>(m_params.samplesPerCycle)) {
        calculateCycleData();
    }

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsNs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_params.frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, 2.0 * std::numbers::pi);

    // UI 갱신 및 사이클 계산을 위한 누적 위상 업데이트
    ++m_sampleCounterForUpdate;

    // 누적된 위상을 보고 Mode에 맞춰 업데이트
    processUpdateByMode(true); // 누적 위상 리셋
    advanceSimulationTime();
}
// -----------------------


// ---- private 함수들 ----
void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsNs);
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage() const
{
    // 1. 기본파 계산
    const double fundamentalPhase = m_currentPhaseRadians + m_params.phaseRadians;
    double voltage = m_params.amplitude * sin(fundamentalPhase);

    // 2. 고조파 계산 (크기가 0보다 클 때만)
    const auto& harmonic = m_params.voltageHarmonic;
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;
        const double harmonicPhase = harmonic.order * fundamentalPhase + harmonicPhaseOffset;
        voltage += harmonic.magnitude * sin(harmonicPhase);
    }

    return voltage;
}

double SimulationEngine::calculateCurrentAmperage() const
{
    // 1. 기본파 계산
    const double fundamentalPhase = m_currentPhaseRadians + m_params.phaseRadians + m_params.currentPhaseOffsetRadians;
    double current = m_params.amplitude * sin(fundamentalPhase);

    // 2. 고조파 계산 (크기가 0보다 클 때만)
    const auto& harmonic = m_params.currentHarmonic;
    if(harmonic.magnitude > 0.0) {
        const double harmonicPhaseOffset = utils::degreesToRadians(harmonic.phase) ;
        const double harmonicPhase = harmonic.order * fundamentalPhase + harmonicPhaseOffset;
        current += harmonic.magnitude * sin(harmonicPhase);
    }

    return current;
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

    // 6. 버퍼 비우기
    m_cycleSampleBuffer.clear();
}

void SimulationEngine::processUpdateByMode(bool resetCounter)
{
    bool shouldEmitUpdate = false;
    const int samplesPerCycle = m_params.samplesPerCycle;

    switch (m_params.updateMode) {
    case UpdateMode::PerSample:
        shouldEmitUpdate = true;
        break;
    case UpdateMode::PerHalfCycle:
        // 샘플 카운터가 한 사이클을 넘으면 업데이트
        if(m_sampleCounterForUpdate >= samplesPerCycle / 2) {
            shouldEmitUpdate = true;
        }
        break;
    case UpdateMode::PerCycle:
        // 샘플 카운터가 한 사이클을 넘으면 업데이트
        if(m_sampleCounterForUpdate >= samplesPerCycle) {
            shouldEmitUpdate = true;
        }
        break;
    }
    if(shouldEmitUpdate) {
        emit dataUpdated(m_data);
        if(resetCounter) {
            // 사용된 만큼만 카운터를 빼서 오차를 줄임
            if(m_params.updateMode == UpdateMode::PerHalfCycle)
                m_sampleCounterForUpdate -= samplesPerCycle / 2;
            else if(m_params.updateMode == UpdateMode::PerCycle)
                m_sampleCounterForUpdate -= samplesPerCycle;
            else
                m_sampleCounterForUpdate = 0;
        }
    }
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
    // 최대 진폭(2/N)을 sqrt(2)로 나누어 실효값(RMS)을 구한는 계수를 구함
    const double normFactor = std::sqrt(2.0) / N;
    return {
        .rms = std::sqrt(squareSum / N),
        .phasorX = normFactor * phasorX_sum,
        .phasorY = normFactor * phasorY_sum
    };
}
