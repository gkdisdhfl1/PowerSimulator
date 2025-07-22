#include "simulation_engine.h"
#include "config.h"
#include <QDebug>
#include <cmath>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_maxDataSize(config::Simulation::DefaultDataSize)
    , m_amplitude(config::Amplitude::Default)
    , m_frequency(config::Frequency::Default) // 기본 주파수 1.0 Hz
    , m_phaseRadians(0.0) // 기본 위상 0
    , m_currentPhse(0.0)
    , m_timeScale(1.0) // 기본 비율은 1.0
    , m_captureIntervalsMs(0.0)
    , m_simulationTimeNs(0)
{
    m_captureTimer.setTimerType(Qt::PreciseTimer);

    double totalSamplesPerSecond = config::Simulation::DefaultSamplingCycles * config::Simulation::DefaultSamplesPerCycle;
    // FPMilliseconds 타입으로 직접 초기화
    m_captureIntervalsMs = FpMilliseconds(1000.0 / totalSamplesPerSecond);

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

void SimulationEngine::applySettings(double interval, int maxSize)
{
    qDebug() << "interval = " << interval;
    m_captureIntervalsMs = FpMilliseconds(interval * 1000); // 기본 간격 저장
    m_maxDataSize = maxSize;
    updateCaptureTimer();

    while(m_data.size() > static_cast<size_t>(m_maxDataSize))
        m_data.pop_front();

    qDebug() << "설정 반영 완료. Interval:" << interval << "s, Max Size:" << maxSize;
}

void SimulationEngine::setAmplitude(double amplitude)
{
    m_amplitude = std::clamp(amplitude, config::Amplitude::Min, config::Amplitude::Max);
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
    m_frequency = std::clamp(hertz, config::Frequency::Min, config::Frequency::Max);
}

void SimulationEngine::updateCaptureTimer()
{
    // 기본 캡처 간격에 시간 비율을 곱해서 실제 타이머 주기를 계산
    const auto scaledInterval = m_captureIntervalsMs * m_timeScale;
    // m_captureTimer.setInterval(static_cast<int>(std::round(m_captureIntervalsMs)));
    m_captureTimer.setInterval(static_cast<int>(std::round(scaledInterval.count())));
}

void SimulationEngine::captureData()
{
    double currentVoltage = calculateCurrentVoltage();
    addNewDataPoint(currentVoltage);

    emit dataUpdated(m_data);

    // 다음 스텝을 위해 현재 진행 위상 업데이트
    const FpSeconds timeDelta = m_captureIntervalsMs;
    const double phaseDelta = 2.0 * std::numbers::pi * m_frequency * timeDelta.count();
    m_currentPhse = std::fmod(m_currentPhse + phaseDelta, 2.0 * std::numbers::pi);

    advanceSimulationTime();
}

void SimulationEngine::advanceSimulationTime()
{
    m_simulationTimeNs += std::chrono::duration_cast<Nanoseconds>(m_captureIntervalsMs);

    // qDebug() << "---------------------------: ";
    // qDebug() << "realIntervalMs (QTimer actual): " << m_captureTimer.interval();
    // qDebug() << "realIntervalMs: " << m_captureIntervalsMs;
    // qDebug() << "step: " << step;
    // qDebug() << "stepInt: " << stepInt;
    // qDebug() << "m_simulationTimeRemainder: " << m_simulationTimeRemainder;

    // 실제 시간이 아닌 시뮬레이션 시간을 증가시킴
}

double SimulationEngine::calculateCurrentVoltage()
{
    const double finalPhase = m_currentPhse + m_phaseRadians;
    return m_amplitude * sin(finalPhase);
}

void SimulationEngine::addNewDataPoint(double voltage)
{
    // DataPoint 객체를 생성하여 저장
    qDebug() << "m_simulationTimeNs: " << m_simulationTimeNs;
    qDebug() << "voltage: " << voltage;
    m_data.push_back({m_simulationTimeNs, voltage});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        m_data.pop_front();
    }
}
