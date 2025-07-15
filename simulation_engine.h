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
    void setPhase(int degrees);
    // void clearData();
    void toggleAutoRotation(bool enabled);
    // void setRotaionSpeed(double speedHz); // 초당 회전수

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void statusChanged(const QString& statusText);
    void voltageChanged(double newVoltage);

private slots:
    void captureData();

private:
    QTimer m_captureTimer;
    QElapsedTimer m_elapsedTimer;

    std::deque<DataPoint> m_data;
    int m_maxDataSize;
    double m_amplitude; // 진폭 (최대 전압)
    double m_phaseRadians; // 위상 (라디안)
    bool m_isAutoRotating = false;
    qint64 m_accumulatedTime; // 총 경과 시간
};

#endif // SIMULATION_ENGINE_H
