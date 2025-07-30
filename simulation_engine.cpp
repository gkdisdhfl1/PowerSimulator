#include "simulation_engine.h"
#include "config.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_maxDataSize(config::Simulation::DefaultDataSize)
    , m_amplitude(config::Source::Amplitude::Default)
    , m_frequency(config::Source::Frequency::Default) // 기본 주파수 1.0 Hz
    , m_timeScale(config::TimeScale::Default) // 기본 비율은 1.0
    , m_samplingCycles(config::Sampling::DefaultSamplingCycles)
    , m_samplesPerCycle(config::Sampling::DefaultSamplesPerCycle)
    , m_phaseRadians(0.0) // 기본 위상 0
    , m_currentPhaseRadians(0.0)
    , m_captureIntervalsMs(0.0)
    , m_simulationTimeNs(0)

{
    using namespace std::chrono_literals;

    m_captureTimer.setTimerType(Qt::PreciseTimer);

    m_captureIntervalsMs = 1.0s / (m_samplingCycles * m_samplesPerCycle);

    connect(&m_captureTimer, &QTimer::timeout, this, &SimulationEngine::captureData);
    updateCaptureTimer(); // 첫 타이머 간격 설정
}

bool SimulationEngine::isRunning() const
{
    return m_captureTimer.isActive();
}
double SimulationEngine::getCaptureIntervalSec() const
{
    return m_captureTimer.interval() / 1000.0;
}
int SimulationEngine::getMaxDataSize() const
{
    return m_maxDataSize;
}
double SimulationEngine::getSamplingCycles() const
{
    return m_samplingCycles;
}
int SimulationEngine::getSamplesPerCycle() const
{
    return m_samplesPerCycle;
}


void SimulationEngine::start()
{
    if (isRunning()) return;
    qDebug() << "시작";
    qDebug() << "m_amplitude = " << m_amplitude << " " << "m_frequency = " << m_frequency;
    qDebug() << "m_maxDataSize = " << m_maxDataSize << "m_timeScale = " << m_timeScale;


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

void SimulationEngine::applySettings(int maxSize)
{
    using namespace std::chrono_literals;

    m_maxDataSize = maxSize;

    while(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        // qDebug() << "--- 이전 데이터 삭제중 ---";
        m_data.pop_front();
    }
    qDebug() << "설정 반영 완료. Max Size: " << maxSize;
}

void SimulationEngine::setAmplitude(double amplitude)
{
    m_amplitude = std::clamp(amplitude, config::Source::Amplitude::Min, config::Source::Amplitude::Max);
}

void SimulationEngine::setPhase(double degrees)
{
    // 각도를 [0, 360) 범위로 정규화
    double normalizedDegrees = std::fmod(degrees, 360.0);
    if (normalizedDegrees < 0) {
        normalizedDegrees += 360.0;
    }

    // 라디안으로 변환하여 내부에 저장
    m_phaseRadians = utils::degreesToRadians(normalizedDegrees);
}

void SimulationEngine::setTimeScale(double scale)
{
    if(scale > 0) {
        m_timeScale = scale;
        updateCaptureTimer(); // 시간 비율이 바뀌었으니 실제 타이머 간격 재설정
    }
}

void SimulationEngine::setFrequency(double hertz)
{
    m_frequency = std::clamp(hertz, config::Source::Frequency::Min, config::Source::Frequency::Max);
}

void SimulationEngine::setSamplingCycles(double samplingCycles)
{
    m_samplingCycles = samplingCycles;
    recalculateCaptureInterval();
}
void SimulationEngine::setSamplesPerCycle(int samplesPerCycle)
{
    m_samplesPerCycle = samplesPerCycle;
    recalculateCaptureInterval();
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    double scaledIntervalMs = (m_captureIntervalsMs * m_timeScale).count();

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

void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    addNewDataPoint(currentVoltage);

    emit dataUpdated(m_data);

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsMs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_frequency * timeDelta.count();
    m_currentPhaseRadians = std::fmod(m_currentPhaseRadians + phaseDelta, 2.0 * std::numbers::pi);

    advanceSimulationTime();
}

void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsMs);
    qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;

}

double SimulationEngine::calculateCurrentVoltage()
{
    const double finalPhase = m_currentPhaseRadians + m_phaseRadians;
    return m_amplitude * sin(finalPhase);
}

void SimulationEngine::addNewDataPoint(double voltage)
{
    // DataPoint 객체를 생성하여 저장
    // qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    // qDebug() << "voltage: " << voltage;
    m_data.push_back({m_simulationTimeNs, voltage});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        // qDebug() << " ---- data{" << m_simulationTimeNs << ", " << voltage << "} 삭제 ----";
        m_data.pop_front();
    }
}

void SimulationEngine::recalculateCaptureInterval()
{
    using namespace std::chrono_literals;

    double totalSamplesPerSecond = m_samplingCycles * m_samplesPerCycle;
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
    // 현재 가지고 있는 m_data를 한번 더 보냄
    if(!m_data.empty())
        emit dataUpdated(m_data);
}
