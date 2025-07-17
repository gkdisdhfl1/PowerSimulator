#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <deque>
#include "data_point.h"

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimulationEngine();

    bool isRunning() const;

    double getCaptureIntervalSec() const; // 현재 설정값을 읽음
    int getMaxDataSize() const;

public slots:
    void start();
    void stop();
    void applySettings(double interval, int maxSize);
    void setAmplitude(double amplitude);
    void setPhase(double degrees);
    void setFrequency(double hertz);
    void setAutoRotation(bool enabled);
    void setTimeScale(double rate);

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void statusChanged(const QString& statusText);
    void phaseUpdated(double newPhase);

private slots:
    void captureData();

private:
    void updateCaptureTimer(); // 내부 헬퍼 함수
    QTimer m_captureTimer;
    // QElapsedTimer m_elapsedTimer;

    std::deque<DataPoint> m_data;
    int m_maxDataSize;
    double m_amplitude; // 진폭 (최대 전압)
    double m_frequency; // 주파수 (Hz)
    double m_phaseDegrees; // 위상 (도)
    qint64 m_accumulatedTime; // 총 경과 시간
    bool m_isAutoRotating;

    double m_timeScale;
    double m_captureIntervalsMs;
    qint64 m_simulationTimeMs;
    double m_simulationTimeRemainder; // 오차 누적을 위한 변수
};

#endif // SIMULATION_ENGINE_H
