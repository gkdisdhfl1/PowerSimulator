#ifndef FREQUENCY_TRACKER_H
#define FREQUENCY_TRACKER_H

#include <QObject>
#include "data_point.h"
#include "measured_data.h"

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

    struct PidCoefficients {
        double Kp = 0.0;
        double Ki = 0.0;
        double Kd = 0.0;
    };

    explicit FrequencyTracker(SimulationEngine* engine, QObject *parent = nullptr);

    void process(const DataPoint& latestDataPoint, const MeasuredData& latestMeasuredData, const std::vector<DataPoint>& cycleBuffer);
    void startTracking();
    void stopTracking();
    TrackingState currentState() const;

    // PID 계수를 외부에서 설정하고 가져오는 함수들
    void setFllCoefficients(const PidCoefficients& coeffs);
    void setZcCoefficients(const PidCoefficients& coeffs);
    PidCoefficients getFllCoefficients() const;
    PidCoefficients getZcCoefficients() const;

signals:
    void samplingCyclesUpdated(double newFrequency);// 자동 추적에 의해 변경될 주파수를 UI에 알리는 시그널

private:
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

    std::vector<DataPoint> m_coarseSearchBuffer; // 데이터 수집용 버퍼
    int m_coarseSearchSamplesNeeded; // 필요한 샘플 개수
    bool m_isVerifying;

    // FLL 관련 변수
    double m_fll_integralError;
    double m_fll_previousFrequencyError;
    int m_fll_failCounter;
    int m_fll_lockCounter;
    double m_fll_previousLfOutput;
    int m_fll_oscillationCounter;

    // FineTune 관련 변수
    double m_pll_previousVoltagePhase;
    int m_pll_failCounter;
    bool m_isFrequencyLocked;
    int m_pll_lockCounter;
    int m_pll_cycleCounter;

    // ZC 추적 관련 변수
    double m_zc_integralError;
    double m_zc_previousPhaseError;

    // PID 계수
    PidCoefficients m_fllCoeffs;
    PidCoefficients m_zcCoeffs;
};

#endif // FREQUENCY_TRACKER_H
