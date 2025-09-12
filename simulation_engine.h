#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <QObject>
#include <QTimer>
#include <deque>
#include "data_point.h"
#include "config.h"
#include "measured_data.h"

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    enum class UpdateMode {
        PerSample,      // 매 샘플마다 갱신
        PerHalfCycle,   // 반 주기마다 갱신
        PerCycle        // 한 주기마다 갱신
    };

    // 주파수 추적 모드를 관리하기 위한 enum
    enum class TrackingState {
        Idle,   // 유휴 상태
        Coarse, // 대략적인 주파수 탐색 (Zero-Crossing)
        FLL_Acquisition, // FLL 상태
        FineTune,    // 정밀한 주파수 추적 (PPL)
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
        double graphWidthSec = config::View::GraphWidth::Default;
        UpdateMode updateMode = UpdateMode::PerSample;
    };

    explicit SimulationEngine();

    bool isRunning() const;
    int getDataSize() const;

    // 파라미터에 직접 접근할 수 있는 인터페이스
    Parameters& parameters();
    const Parameters& parameters() const;

public slots:
    void start();
    void stop();
    void onRedrawRequest();
    void onRedrawAnalysisRequest();
    void onMaxDataSizeChanged(int newSize);
    void updateCaptureTimer();
    void recalculateCaptureInterval();
    void enableFrequencyTracking(bool enabled); // 자동 주파수 추적 활성화/바활성화 슬롯

signals:
    void dataUpdated(const std::deque<DataPoint>& data);
    void runningStateChanged(bool isRunning);
    void measuredDataUpdated(const std::deque<MeasuredData>& data);
    void samplingCyclesUpdated(double newFrequency); // 자동 추적에 의해 변경될 주파수를 UI에 알리는 시그널

private slots:
    void captureData();

private:
    using FpMilliseconds = utils::FpMilliseconds;
    using Nanoseconds = utils::Nanoseconds;
    using FpSeconds = utils::FpSeconds;

    struct CycleMetrics {
        double rms;
        double phasorX;
        double phasorY;
    };
    enum class DataType { Voltage, Current};
    CycleMetrics calculateMetricsFor(DataType type) const;

    void advanceSimulationTime();
    double calculateCurrentVoltage() const;
    double calculateCurrentAmperage() const;
    void addNewDataPoint(double voltage, double current);
    void calculateCycleData(); // RMS, 전력 계산 함수
    void processUpdateByMode(bool resetAccumulatedPhase);

    // 자동 추적 관련 함수들
    void startCoarseSearch(); // 거친 탐색을 시작하는 헬퍼 함수
    void processCoarseSearch(); // 거친 탐색 데이터 수집 및 분석
    void processFineTune(); // PLL 로직 처리 함수
    double estimateFrequencyByZeroCrossing(); // zero-crossing 주파수 계산 함수
    void checkFrequencyLock(double phaseError); // Lock 감지 헬퍼
    void processFll(double phaseError);
    void checkFllLock(double frequencyError);
    void startVerification(); // 검증 시작 함수
    void processVerification(); // 검증 처리 함수

    QTimer m_captureTimer;
    std::deque<DataPoint> m_data;
    Parameters m_params;

    double m_currentPhaseRadians; // 현재 누적 위상
    double m_accumulatedPhaseSinceUpdate; // 마지막 갱신 후 누적된 위상 변화량
    FpMilliseconds m_captureIntervalsMs; // 기본 캡처 간격 (double, ms)
    Nanoseconds m_simulationTimeNs; // 시뮬레이션 누적 시간 (정수, ns)

    // measuredData 관련 변수
    std::deque<MeasuredData> m_measuredData; // 계산된 데이터를 저장할 컨테이너
    std::vector<DataPoint> m_cycleSampleBuffer; // 1사이클 동안의 샘플을 모으는 버퍼

    // PLL 관련 멤버 변수
    double m_previousVoltagePhase; // 이전 사이클의 전압 위상
    double m_integralError; // 위상 오차의 누적값
    int m_fineTuneFailCounter; // 실패 카운터
    bool m_isFrequencyLocked; // 주파수 고정 플래그
    int m_frequencyLockCounter; // 주파수 고정 감지 카운터

    // ZC 추적용 멤버 변수
    double m_phaseIntegralError; // 위상 오차 누적값
    double m_previousZcPhaseError;

    // FLL 관련 변수
    double m_previousFrequencyError;
    int m_fllFailCounter; // FLL 실패 카운터
    double m_previousLfOutput; // 이전 제어기 출력을 저장할 변수
    int m_oscillationCounter; // 진동 횟수 카운터

    // CoarseSearch 용 변수
    std::vector<DataPoint> m_coarseSearchBuffer; // 데이터 수집용 버퍼
    int m_coarseSearchSamplesNeeded; // 필요한 샘플 개수
    TrackingState m_trackingState;
    int m_fineTuneCycleCounter; // FineTune 실패 카운터
    bool m_isVerifying;
};

#endif // SIMULATION_ENGINE_H
