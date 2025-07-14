#include "simulation_engine.h"
#include "config.h"
#include <QDebug>

SimulationEngine::SimulationEngine()
    : QObject()
    , m_maxDataSize(config::DefaultDataSize)
    , m_amplitude(config::DefaultVoltage)
    , m_phaseRadians(0.0) // 기본 위상 0
    , m_accumulatedTime(0)
{
    m_captureTimer.setInterval(config::DefaultIntervalMs); // 초기값
    connect(&m_captureTimer, &QTimer::timeout, this, &SimulationEngine::captureData);
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

    if (!m_elapsedTimer.isValid()) {
        m_elapsedTimer.start();
    }
    m_captureTimer.start();
    emit statusChanged("일시정지");
    qDebug() << "Engine started.";
}

void SimulationEngine::stop()
{
    if (!isRunning()) return;

    m_captureTimer.stop();
    m_accumulatedTime += m_elapsedTimer.elapsed();
    m_elapsedTimer.invalidate(); // 타이머 정지

    emit statusChanged("시작");
    qDebug() << "Engine stopped.";
}

void SimulationEngine::applySettings(double interval, int maxSize)
{
    m_captureTimer.setInterval(static_cast<int>(interval * 1000));
    m_maxDataSize = maxSize;

    while(m_data.size() > static_cast<size_t>(m_maxDataSize))
        m_data.pop_front();

    qDebug() << "설정 반영 완료. Interval:" << interval << "s, Max Size:" << maxSize;
}

void SimulationEngine::setAmplitude(double amplitude)
{
    m_amplitude = std::clamp(amplitude, config::MinVoltage, config::MaxVoltage);
}

void SimulationEngine::setPhase(int degrees)
{
    // 각도를 라디안으로 변환하여 저장
    // 360도 = 2 * pi 라디안
    m_phaseRadians = static_cast<double>(degrees) * (2.0 * config::PI) / 360.0;
}

void SimulationEngine::captureData()
{
    // 데이터 생성
    qint64 currentTimeMs = m_elapsedTimer.elapsed() + m_accumulatedTime;

    // AC 전압 계산 V = A * sin(2 * pi * f * t + phase)
    double currentVoltage = m_amplitude * sin(m_phaseRadians);

    // DataPoint 객체를 생성하여 저장
    m_data.push_back({currentTimeMs, currentVoltage});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        m_data.pop_front();
    }

    emit dataUpdated(m_data);
}
