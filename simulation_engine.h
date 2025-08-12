#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <QObject>
#include <QTimer>
#include <deque>
#include "data_point.h"
#include "config.h"

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    enum class UpdateMode {
        PerSample,      // 매 샘플마다 갱신
        PerHalfCycle,   // 반 주기마다 갱신
        PerCycle        // 한 주기마다 갱신
    };

    // 시뮬레이션 매개변수를 담는 구조체
    struct Parameters {
        double amplitude = config::Source::Amplitude::Default;
        double currentAmplitude = config::Source::Current::DefaultAmplitude;
        double frequency = config::Source::Frequency::Default;
        double phaseRadians = 0.0;
        double currentPhaseOffsetRadians = 0.0;
        double timeScale = config::TimeScale::Default;
        double samplingCycles = config::Sampling::DefaultSamplingCycles;
        double samplesPerCycle = config::Sampling::DefaultSamplesPerCycle;
        int maxDataSize = config::Simulation::DefaultDataSize;
        UpdateMode updateMode = UpdateMode::PerSample;
    };

    explicit SimulationEngine();

    bool isRunning() const;

    // 파라미터에 직접 접근할 수 있는 인터페이스
    Parameters& parameters();
    const Parameters& parameters() const;

public slots:
    void start();
    void stop();
    void onRedrawRequest();
    void updateCaptureTimer();
    void recalculateCaptureInterval();

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
    double calculateCurrentAmperage();
    void addNewDataPoint(double voltage, double current);

    QTimer m_captureTimer;
    std::deque<DataPoint> m_data;
    Parameters m_params;

    double m_currentPhaseRadians; // 현재 누적 위상
    double m_accumulatedPhaseSinceUpdate; // 마지막 갱신 후 누적된 위상 변화량
    FpMilliseconds m_captureIntervalsMs; // 기본 캡처 간격 (double, ms)
    Nanoseconds m_simulationTimeNs; // 시뮬레이션 누적 시간 (정수, ns)
};

#endif // SIMULATION_ENGINE_H
