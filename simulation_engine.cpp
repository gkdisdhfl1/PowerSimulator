#include "simulation_engine.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_currentPhaseRadians(0.0)
    , m_accumulatedPhaseSinceUpdate(0.0)
    , m_captureIntervalsMs(0.0)
    , m_simulationTimeNs(0)
{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsMs = 1.0s / (m_params.samplingCycles * m_params.samplesPerCycle);

    connect(&m_captureTimer, &QTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정
}

bool SimulationEngine::isRunning() const { return m_captureTimer.isActive(); }
SimulationEngine::Parameters& SimulationEngine::parameters() { return m_params; };
const SimulationEngine::Parameters& SimulationEngine::parameters() const { return m_params; };


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

void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    double currentAmperage = calculateCurrentAmperage();
    addNewDataPoint(currentVoltage, currentAmperage);

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsMs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_params.frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, 2.0 * std::numbers::pi);

    // 마지막 갱신 후 누적된 위상 변화량도 업데이트
    m_accumulatedPhaseSinceUpdate += phaseDelta;

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
        if(m_params.updateMode == UpdateMode::PerHalfCycle)
            m_accumulatedPhaseSinceUpdate -= std::numbers::pi;
        else if(m_params.updateMode == UpdateMode::PerCycle)
            m_accumulatedPhaseSinceUpdate -= 2.0 * std::numbers::pi;
        else
            m_accumulatedPhaseSinceUpdate = 0.0;
    }
    advanceSimulationTime();
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


void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsMs);
    qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage()
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians;
    return m_params.amplitude * sin(finalPhase);
}

double SimulationEngine::calculateCurrentAmperage()
{
    const double finalPhase = m_currentPhaseRadians + m_params.phaseRadians + m_params.currentPhaseOffsetRadians;
    return m_params.currentAmplitude * sin(finalPhase);
}

void SimulationEngine::addNewDataPoint(double voltage, double current)
{
    // DataPoint 객체를 생성하여 저장
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    qDebug() << "voltage: " << voltage;
    qDebug() << "current: " << current;
    m_data.push_back({m_simulationTimeNs, voltage, current});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_params.maxDataSize)) {
        // qDebug() << " ---- data{" << m_simulationTimeNs << ", " << voltage << "} 삭제 ----";
        m_data.pop_front();
    }
}

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_params.samplingCycles * m_params.samplesPerCycle;
    qDebug() << "totalSamplesPerSecond: " << totalSamplesPerSecond;

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
    // bool shouldEmitUpdate = false;

    // // 현재 가지고 있는 m_data를 한번 더 보냄
    // switch (m_params.updateMode) {
    // case UpdateMode::PerSample:
    //     shouldEmitUpdate = true;
    //     break;
    // case UpdateMode::PerHalfCycle:
    //     // 누적된 위상이 PI(180도) 이상 변했는지 확인
    //     if(m_accumulatedPhaseSinceUpdate >= std::numbers::pi) {
    //         shouldEmitUpdate = true;
    //     }
    //     break;
    // case UpdateMode::PerCycle:
    //     // 누적된 위상이 2*PI 이상 변했는지 확인
    //     if(m_accumulatedPhaseSinceUpdate >= 2.0 * std::numbers::pi) {
    //         shouldEmitUpdate = true;
    //     }
    //     break;
    // }
    // if(shouldEmitUpdate) {
    //     emit dataUpdated(m_data);
    // }
    emit dataUpdated(m_data);
}
