#include "simulation_engine.h"
#include "config.h"
#include <QDebug>
#include <algorithm> // for std::clamp

namespace {
    constexpr int FullCircleDegrees = 360;
    constexpr int HalfCircleDegrees = 180;
}

SimulationEngine::SimulationEngine()
    : QObject()
    , m_maxDataSize(config::DefaultDataSize)
    , m_currentVoltageValue(config::DefaultVoltage)
    , m_lastDialValue(0)
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

void SimulationEngine::updateVoltage(int newDialValue)
{
    int diff = newDialValue - m_lastDialValue;

    // Wrapping 처리
    if(diff < -HalfCircleDegrees)
        diff += FullCircleDegrees;
    else if(diff > HalfCircleDegrees)
        diff -= FullCircleDegrees;

    double nextVoltage = m_currentVoltageValue + static_cast<double>(diff);

    // 실제 전압 값 업데이트
    m_currentVoltageValue = std::clamp(nextVoltage, config::MinVoltage, config::MaxVoltage);

    emit voltageChanged(m_currentVoltageValue);

    // 다음 계산을 위한 현재 위치 저장
    m_lastDialValue = newDialValue;
}

void SimulationEngine::setCurrentVoltage(double voltage)
{
    m_currentVoltageValue = std::clamp(voltage, config::MinVoltage, config::MaxVoltage);
}

void SimulationEngine::captureData()
{
    // 데이터 생성
    qint64 currentTimeMs = m_elapsedTimer.elapsed() + m_accumulatedTime;
    double currentVoltage = m_currentVoltageValue;

    // DataPoint 객체를 생성하여 저장
    m_data.push_back({currentTimeMs, currentVoltage});

    // 최대 개수 관리
    if(m_data.size() > static_cast<size_t>(m_maxDataSize)) {
        m_data.pop_front();
    }

    emit dataUpdated(m_data);
}
