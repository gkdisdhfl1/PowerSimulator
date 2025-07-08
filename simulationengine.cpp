#include "simulationengine.h"
#include <QDebug>
#include <algorithm> // For std::clamp

namespace {
    constexpr int FullCircleDegrees = 360;
    constexpr int HalfCircleDegrees = 180;
}

SimulationEngine::SimulationEngine(QObject *parent)
    : QObject(parent)
    , m_maxDataSize(100)
    , m_currentVoltageValue(220.0)
    , m_lastDialValue(0)
{
    m_captureTimer.setInterval(100); // 초기값 100ms
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
    emit statusChanged("시작");
    qDebug() << "Engine stopped.";
}

void SimulationEngine::applySettings(double interval, int maxSize)
{
    m_captureTimer.setInterval(static_cast<int>(interval * 1000));
    m_maxDataSize = maxSize;

    while(m_data.size() > static_cast<size_t>(m_maxDataSize))
        m_data.pop_front();

    qDebug() << "Engine settings applied. Interval:" << interval << "s, Max Size:" << maxSize;
}

void SimulationEngine::updateVoltage(int newDialValue)
{
    int diff = newDialValue - m_lastDialValue;

    if(diff < -HalfCircleDegrees)
        diff += FullCircleDegrees;
    else if(diff > HalfCircleDegrees)
        diff -= FullCircleDegrees;

    double nextVoltage = m_currentVoltageValue + static_cast<double>(diff);

    // Min/Max는 UI에 있으므로 여기서는 일단 넓은 범위로 제한
    m_currentVoltageValue = std::clamp(nextVoltage, -500.0, 500.0);

    emit voltageChanged(m_currentVoltageValue);

    m_lastDialValue = newDialValue;
}

void SimulationEngine::setCurrentVoltage(double voltage)
{
    m_currentVoltageValue = voltage;
}

void SimulationEngine::captureData()
{
    qint64 currentTimeMs = m_elapsedTimer.elapsed();
    double currentVoltage = m_currentVoltageValue;

    m_data.push_back(DataPoint{currentTimeMs, currentVoltage});

    if(m_data.size() > m_maxDataSize) {
        m_data.pop_front();
    }

    emit dataUpdated(m_data);
}
