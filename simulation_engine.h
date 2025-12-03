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

// SimulationEngine 클래스
// PowerSimulator의 핵심 로직 담당.
// 시뮬레이션 루프 관리, 3상 전압/전류 신호 생성,
// 실시간 분석(RMS, 전력, 고조파 등) 수행.
// 타이머(m_captureTimer)에 의해 주기적으로 데이터 포인트 생성하고 UI 신호 보냄.
class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    // --- 시뮬레이션 매개변수 (Properties) ---
    Property<double> m_amplitude;               // 기본 진폭
    Property<double> m_currentAmplitude;        // 기본 전류 진폭
    Property<double> m_frequency;               // 기본 주파수 (Hz)
    Property<double> m_phaseRadians;            // 전체 위상 변화 (라디안)
    Property<double> m_currentPhaseOffsetRadians; // 전압 대비 전류 위상 오프셋
    Property<double> m_timeScale;               // 시간 스케일 비율
    Property<double> m_samplingCycles;          // 샘플링할 주기 수
    Property<int> m_samplesPerCycle;            // 주기당 샘플 수
    Property<int> m_maxDataSize;                // 데이터 포인트 버퍼 최대 크기
    Property<double> m_graphWidthSec;           // 그래프 가로 폭 (초 단위)
    Property<UpdateMode> m_updateMode;          // 데이터 업데이트 모드

    // --- 고조파 설정 ---
    Property<HarmonicComponent> m_voltageHarmonic; // 전압 고조파 설정
    Property<HarmonicComponent> m_currentHarmonic; // 전류 고조파 설정

    // --- 3상 설정 ---
    // 전압
    Property<double> m_voltage_B_amplitude;
    Property<double> m_voltage_B_phase_deg;
    Property<double> m_voltage_C_amplitude;
    Property<double> m_voltage_C_phase_deg;

    // 전류
    Property<double> m_current_B_amplitude;
    Property<double> m_current_B_phase_deg;
    Property<double> m_current_C_amplitude;
    Property<double> m_current_C_phase_deg;
// ---------------------------------------------

    explicit SimulationEngine();

    // 시뮬레이션이 현재 실행 중인지 확인
    bool isRunning() const;

    // 현재 데이터 버퍼의 크기 반환
    int getDataSize() const;

    FrequencyTracker* getFrequencyTracker() const;

public slots:
    // 시뮬레이션 루프 시작
    void start();

    // 시뮬레이션 루프 정지
    void stop();

    void onRedrawRequest();
    void onRedrawAnalysisRequest();
    void onMaxDataSizeChanged(int newSize);
    void updateCaptureTimer();
    void recalculateCaptureInterval();
    void enableFrequencyTracking(bool enabled);

signals:
    // 새로운 원시 파형 데이터가 준비되었을 때 발생
    void dataUpdated(const std::deque<DataPoint>& data);

    // 실행 상태가 변경되었을 때 발생 (시작/정지)
    void runningStateChanged(bool isRunning);

    // 계산된 측정 데이터(RMS, 전력 등)가 업데이트되었을 때 발생
    void measuredDataUpdated(const std::deque<MeasuredData>& data);
      
    // 1초마다 요약된 데이터가 업데이트되었을 때 발생
    void oneSecondDataUpdated(const OneSecondSummaryData& data);

    // 페이저 분석 데이터가 업데이트되었을 때 발생
    void phasorUpdated(const GenericPhaseData<HarmonicAnalysisResult>& fundamentalVoltage,
                       const GenericPhaseData<HarmonicAnalysisResult>& fundamentalCurrent,
                       const std::vector<HarmonicAnalysisResult>& voltageHarmonics,
                       const std::vector<HarmonicAnalysisResult>& currentHarmonics);

private slots:
    // 메인 시뮬레이션 단계. m_captureTimer에 의해 호출됨.
    // 새로운 데이터 포인트를 생성하고 처리.
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
    
    // 현재 주기에 대한 RMS, 전력 및 기타 지표를 계산
    void calculateCycleData(); 
    
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
