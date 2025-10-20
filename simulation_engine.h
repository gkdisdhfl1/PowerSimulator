#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <QObject>
#include <QChronoTimer>
#include <deque>
#include <complex>
#include "analysis_utils.h"
#include "data_point.h"
#include "config.h"
#include "measured_data.h"
#include "shared_data_types.h"

class FrequencyTracker;

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
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
        double graphWidthSec = config::View::GraphWidth::Default;
        UpdateMode updateMode = UpdateMode::PerSample;
        HarmonicComponent voltageHarmonic = {config::Harmonics::DefaultOrder, config::Harmonics::DefaultMagnitude, config::Harmonics::DefaultPhase};
        HarmonicComponent currentHarmonic = {config::Harmonics::DefaultOrder, config::Harmonics::DefaultMagnitude, config::Harmonics::DefaultPhase};

        // -- 3상 설정을 위한 매개변수 ---
        // 전압
        double voltage_B_amplitude = config::Source::ThreePhase::DefaultAmplitudeB;
        double voltage_B_phase_deg = config::Source::ThreePhase::DefaultPhaseB_deg;
        double voltage_C_amplitude = config::Source::ThreePhase::DefaultAmplitudeC;
        double voltage_C_phase_deg = config::Source::ThreePhase::DefaultPhaseC_deg;

        // 전류
        double current_B_amplitude = config::Source::ThreePhase::DefaultCurrentAmplitudeB;
        double current_B_phase_deg = config::Source::ThreePhase::DefaultCurrentPhaseB_deg;
        double current_C_amplitude = config::Source::ThreePhase::DefaultCurrentAmplitudeC;
        double current_C_phase_deg = config::Source::ThreePhase::DefaultCurrentPhaseC_deg;
    };

    explicit SimulationEngine();

    bool isRunning() const;
    int getDataSize() const;

    // 파라미터에 직접 접근할 수 있는 인터페이스
    Parameters& parameters();
    const Parameters& parameters() const;

    FrequencyTracker* getFrequencyTracker() const;

public slots:
    void start();
    void stop();
    void onRedrawRequest();
    void onRedrawAnalysisRequest();
    void onMaxDataSizeChanged(int newSize);
    void updateCaptureTimer();
    void recalculateCaptureInterval();
    void enableFrequencyTracking(bool enabled);

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void runningStateChanged(bool isRunning);
    void measuredDataUpdated(const std::deque<MeasuredData>& data);
    void samplingCyclesUpdated(double newFrequency);
    void oneSecondDataUpdated(const OneSecondSummaryData& data);

private slots:
    void captureData();

private:
    friend FrequencyTracker;

    using FpNanoseconds = utils::FpNanoseconds;
    using Nanoseconds = utils::Nanoseconds;
    using FpSeconds = utils::FpSeconds;

    std::expected<std::vector<std::complex<double>>, AnalysisUtils::SpectrumError> analyzeSpectrum(AnalysisUtils::DataType type, int phase) const;

    void advanceSimulationTime();
    PhaseData calculateCurrentVoltage() const;
    PhaseData calculateCurrentAmperage() const;
    void addNewDataPoint(PhaseData voltage, PhaseData current);
    void calculateCycleData(); // RMS, 전력 계산 함수
    void processUpdateByMode(bool resetCounter);
    void processOneSecondData(const MeasuredData& latestCycleData);

    QChronoTimer m_captureTimer;
    std::deque<DataPoint> m_data;
    Parameters m_params;

    double m_currentPhaseRadians; // 현재 누적 위상
    int m_sampleCounterForUpdate;
    FpNanoseconds m_captureIntervalsNs; // 기본 캡처 간격 (double, ns)
    Nanoseconds m_simulationTimeNs; // 시뮬레이션 누적 시간 (정수, ns)

    // measuredData 관련 변수
    std::deque<MeasuredData> m_measuredData; // 계산된 데이터를 저장할 컨테이너
    std::vector<DataPoint> m_cycleSampleBuffer; // 1사이클 동안의 샘플을 모으는 버퍼

    std::unique_ptr<FrequencyTracker> m_frequencyTracker;

    // 1초 데이터 관련 변수
    std::vector<MeasuredData> m_oneSecondCycleBuffer;
    Nanoseconds m_oneSecondBlockStartTime;
    double m_totalEngeryWh;
};

#endif // SIMULATION_ENGINE_H
