#include "simulation_engine.h"
#include "config.h"
#include <QDebug>
#include <cmath>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_maxDataSize(config::Simulation::DefaultDataSize)
    , m_amplitude(config::Amplitude::Default)
    , m_frequency(config::Frequency::Default) // 기본 주파수 1.0 Hz
    , m_phaseDegrees(0.0) // 기본 위상 0
    , m_accumulatedTime(0)
    , m_timeScale(1.0) // 기본 비율은 1.0
    , m_captureIntervalsMs(config::Simulation::DefaultIntervalMs) // 기본 시뮬레이션 간격
    , m_simulationTimeMs(0) // 시뮬레이션 시간은 0에서 시작
    , m_simulationTimeRemainder(0.0)
{
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
    m_simulationTimeMs = 0; // 시뮬레이션 시작 시 시간 초기화
    m_data.clear(); // 데이터도 초기화
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
    m_captureTimer.setInterval(static_cast<int>(interval * 1000));
    m_maxDataSize = maxSize;
    // updateCaptureTimer();

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
    m_phaseDegrees = std::fmod(degrees, 360.0);
    if (m_phaseDegrees < 0) {
        m_phaseDegrees += 360.0;
    }
}

void SimulationEngine::setFrequency(double hertz)
{
    if (hertz >= 0) {
        m_frequency = hertz;
    }
}

void SimulationEngine::captureData()
{
    double realIntervalMs = static_cast<double>(m_captureTimer.interval());
    double simulationStepDouble = realIntervalMs / m_timeScale;
    simulationStepDouble += m_simulationTimeRemainder;
    qint64 simulationStepInt = static_cast<qint64>(simulationStepDouble);
    m_simulationTimeRemainder = simulationStepDouble - static_cast<double>(simulationStepInt);


    // 위상 업데이트
    double stepSec = (simulationStepDouble - m_simulationTimeRemainder) / 1000.0;
    double degreesPerInterval = m_frequency * 360.0 * stepSec;
    m_phaseDegrees += degreesPerInterval;
    m_phaseDegrees = std::fmod(m_phaseDegrees, 360.0);
    emit phaseUpdated(m_phaseDegrees);


    // 실제 시간이 아닌 시뮬레이션 시간을 증가시킴
    m_simulationTimeMs += simulationStepInt;

    // AC 전압 계산 V = A * sin(phase)
    // 위상을 라디안으로 변환하여 계산
    double phaseRadians = utils::degreesToRadians(m_phaseDegrees);
    double currentVoltage = m_amplitude * sin(phaseRadians);

    // DataPoint 객체를 생성하여 저장
    m_data.push_back({m_simulationTimeMs, currentVoltage});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        m_data.pop_front();
    }

    emit dataUpdated(m_data);
}

void SimulationEngine::setTimeScale(double scale)
{
    if(scale > 0) {
        m_timeScale = scale;
        // updateCaptureTimer(); // 시간 비율이 바뀌었으니 실제 타이머 간격 재설정
    }
}

void SimulationEngine::updateCaptureTimer()
{
    double realIntervalMs = m_captureIntervalsMs * m_timeScale;
    m_captureTimer.setInterval(static_cast<int>(std::round(realIntervalMs)));
}
