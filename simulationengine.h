#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <deque>
#include "datapoint.h"

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimulationEngine(QObject *parent = nullptr);

    bool isRunning() const;

    // Getters for current settings
    double getCaptureIntervalSec() const;
    int getMaxDataSize() const;

public slots:
    void start();
    void stop();
    void applySettings(double interval, int maxSize);
    void updateVoltage(int newDialValue);
    void setCurrentVoltage(double voltage);

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
    double m_currentVoltageValue;
    int m_lastDialValue;
};

#endif // SIMULATIONENGINE_H
