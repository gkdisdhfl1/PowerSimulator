#ifndef FREQUENCY_TRACKER_H
#define FREQUENCY_TRACKER_H

#include <QObject>
#include <expected>
#include "data_point.h"
#include "measured_data.h"
#include "pid_controller.h"

// SimulationEngine 전방선언
struct SimulationEngine;

class FrequencyTracker : public QObject
{
    Q_OBJECT
public:
    // 주파수 추적 모드를 관리하기 위한 enum
    enum class TrackingState {
        Idle,   // 유휴 상태
        Coarse, // 대략적인 주파수 탐색 (Zero-Crossing)
        FLL_Acquisition, // FLL 상태
        FineTune,    // 정밀한 주파수 추적 (PPL)
    };

    using PidCoefficients = PIDController::Coefficients;

    explicit FrequencyTracker(SimulationEngine* engine, QObject *parent = nullptr);

    void process(const DataPoint& latestDataPoint, const MeasuredData& latestMeasuredData, const std::vector<DataPoint>& cycleBuffer);
    void startTracking();
    void stopTracking();
    TrackingState currentState() const;

    // PID 계수를 외부에서 설정하고 가져오는 함수들
    void setFllCoefficients(const PidCoefficients& coeffs);
    void setZcCoefficients(const PidCoefficients& coeffs);
    PidCoefficients getFllCoefficients();
    PidCoefficients getZcCoefficients();

    std::vector<int> getRequiredValues();

signals:
    void samplingCyclesUpdated(double newFrequency);// 자동 추적에 의해 변경될 주파수를 UI에 알리는 시그널

private:
    enum class PhaseErrorType {
        noError,
        NoFundamentalComponent,
        FirstRun
    };

    struct PhaseInfo {
        double currentAngle;
        double previousAngle;
        double error;
    };

    // 위상차를 계산하고, 실패 원인을 함께 반환하는 함수
    std::expected<PhaseInfo, PhaseErrorType> calculatePhaseError(const MeasuredData& latestMeasuredData);

    // --- 상태 처리 함수  ---
    void processCoarseSearch(const DataPoint& latestDataPoint); // 거친 탐색 데이터 수집 및 분석
    void processFll(const MeasuredData& latestMeasuredData);
    void processFineTune(const MeasuredData& latestMeasuredData); // PLL 로직 처리 함수
    void processVerification(const DataPoint& latestDataPoint); // 검증 처리 함수

    // --- 헬퍼 함수 ---
    void startCoarseSearch(); // 거친 탐색을 시작하는 헬퍼 함수
    void resetAllStates();
    double estimateFrequencyByZeroCrossing(const std::vector<double>& wave); // zero-crossing 주파수 계산 함수
    void checkFllLock(double frequencyError);
    void startVerification();

    // --- 멤버 변수 ---
    SimulationEngine* m_engine; // 엔진에 대한 포인터 (소유권 없음)
    TrackingState m_trackingState;

    // CoarseSearch 관련 변수
    std::vector<DataPoint> m_coarseSearchBuffer; // 데이터 수집용 버퍼
    int m_coarseSearchSamplesNeeded; // 필요한 샘플 개수
    bool m_isVerifying;

    // PID 계수
    PIDController m_fllController;
    PIDController m_zcController;

    // FLL/PLL 상태 관련 변수
    int m_fll_failCounter;
    int m_fll_lockCounter;
    double m_fll_previousLfOutput;
    int m_fll_oscillationCounter;

    double m_pll_previousVoltagePhase;
    int m_pll_failCounter;
    int m_pll_cycleCounter;

};

#endif // FREQUENCY_TRACKER_H
