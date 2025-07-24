#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <QObject>
#include <QTimer>
#include <deque>
#include "data_point.h"

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimulationEngine();

    bool isRunning() const;

    double getSamplingCycles() const;
    int getSamplesPerCycle() const;
    int getMaxDataSize() const;
    double getCaptureIntervalSec() const; // 현재 설정값을 읽음

public slots:
    void start();
    void stop();
    void applySettings(int maxSize);
    void setAmplitude(double amplitude);
    void setPhase(double degrees);
    void setFrequency(double hertz);
    void setTimeScale(double rate);
    void setSamplingcycles(double samplingCycles);
    void setSamplesPerCycle(int samplesPerCycle);

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void runningStateChanged(bool isRunning);

private slots:
    void captureData();

private:
    using FpSeconds = std::chrono::duration<double>;
    using FpMilliseconds = std::chrono::duration<double, std::milli>;
    using Nanoseconds = std::chrono::nanoseconds;

    void advanceSimulationTime();
    double calculateCurrentVoltage();
    void addNewDataPoint(double voltage);
    void updateCaptureTimer();
    void recalculateCaptureInterval();

    QTimer m_captureTimer;

    std::deque<DataPoint> m_data;
    int m_maxDataSize;
    double m_amplitude; // 진폭 (최대 전압)
    double m_frequency; // 주파수 (Hz)
    double m_phaseRadians; // 위상 (라디안)
    double m_currentPhaseRadians; // 현재 누적 위상

    double m_samplingCycles;
    double m_samplesPerCycle;
    double m_timeScale;
    FpMilliseconds m_captureIntervalsMs; // 기본 캡처 간격 (double, ms)
    Nanoseconds m_simulationTimeNs; // 시뮬레이션 누적 시간 (정수, ns)
};

#endif // SIMULATION_ENGINE_H
