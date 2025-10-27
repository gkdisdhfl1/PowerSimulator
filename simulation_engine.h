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
#include "Property.h"

class FrequencyTracker;

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
   // 시뮬레이션 매개변수를 담는 구조체
    struct Parameters {
        Property<double>* amplitude;
        Property<double>* currentAmplitude;
        Property<double>* frequency;
        Property<double>* phaseRadians;
        Property<double>* currentPhaseOffsetRadians;
        Property<double>* timeScale;
        Property<double>* samplingCycles;
        Property<int>* samplesPerCycle;
        Property<int>* maxDataSize;
        Property<double>* graphWidthSec;
        Property<UpdateMode>* updateMode;

        // 고조파 설정
        Property<HarmonicComponent>* voltageHarmonic;
        Property<HarmonicComponent>* currentHarmonic;

        // 3상 설정
        // 전압
        Property<double>* voltage_B_amplitude;
        Property<double>* voltage_B_phase_deg;
        Property<double>* voltage_C_amplitude;
        Property<double>* voltage_C_phase_deg;

        // 전류
        Property<double>* current_B_amplitude;
        Property<double>* current_B_phase_deg;
        Property<double>* current_C_amplitude;
        Property<double>* current_C_phase_deg;
    };

    explicit SimulationEngine();

    bool isRunning() const;
    int getDataSize() const;

    FrequencyTracker* getFrequencyTracker() const;
    Parameters m_params;

public slots:
    void start();
    void stop();
    void onRedrawRequest();
    void onRedrawAnalysisRequest();
    void onMaxDataSizeChanged(int newSize); // 추후 정리
    void updateCaptureTimer();
    void recalculateCaptureInterval();
    void enableFrequencyTracking(bool enabled);

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void runningStateChanged(bool isRunning);
    void measuredDataUpdated(const std::deque<MeasuredData>& data);
    // void samplingCyclesUpdated(double newFrequency); // 추후 정리
    void oneSecondDataUpdated(const OneSecondSummaryData& data);

private slots:
    void captureData();
    void handleMaxDataSizeChange(int newSize);

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
