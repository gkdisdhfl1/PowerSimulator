#ifndef MEASURED_DATA_H
#define MEASURED_DATA_H

#include "shared_data_types.h"
#include <QMetaType>
#include <chrono>

// 단일 고조파 성분의 분석 결과를 담는 구조체
struct HarmonicAnalysisResult {
    int order = 0;      // 차수
    double rms = 0.0;     // RMS 크기
    double phase = 0.0;   // 위상(라디안)
    double phasorX = 0.0;  // cos 성분(실수)
    double phasorY = 0.0;  // sin 성분(허수)
};

// 한 사이클 동안 연산 결과를 담는 구조체
struct MeasuredData {
    std::chrono::nanoseconds timestamp; // 사이클이 끝나는 시점의 타임스탬프
    PhaseData voltageRms; // 전압 RMS
    PhaseData currentRms; // 전류 RMS
    PhaseData activePower; // 유효 전력

    // 3상 분석 결과
    std::array<HarmonicAnalysisResult, 3> fundamentalVoltage; // [0]:A [1]:B [2]:C
    std::array<HarmonicAnalysisResult, 3> fundamentalCurrent;
    std::array<HarmonicAnalysisResult, 3> dominantVoltage;
    std::array<HarmonicAnalysisResult, 3> dominantCurrent;

    // 전압의 주파수 성분 분석 결과 (harmonics[0]: 기본파)
    std::vector<HarmonicAnalysisResult> voltageHarmonics;
    std::vector<HarmonicAnalysisResult> voltageHarmonicsB;
    std::vector<HarmonicAnalysisResult> voltageHarmonicsC;

    // 전압의 주파수 성분 분석 결과 (harmonics[0]: 기본파)
    std::vector<HarmonicAnalysisResult> currentHarmonics;
    std::vector<HarmonicAnalysisResult> currentHarmonicsB;
    std::vector<HarmonicAnalysisResult> currentHarmonicsC;

    // 잔류 RMS 멤버
    double residualVoltageRms = 0.0;
    double residualCurrentRms = 0.0;
};

// 단일 시퀀스 성분
struct SymmetricalComponent {
    double magnitude = 0.0;
    double phase_deg = 0.0;
};

// 영상/정상/역상분 집합
struct SymmetricalComponents {
    SymmetricalComponent zero;
    SymmetricalComponent positive;
    SymmetricalComponent negative;
};

// 1초 단위로 가공된 분석 데이터를 담는 구조체
struct OneSecondSummaryData {
    PhaseData totalVoltageRms;
    PhaseData totalCurrentRms;
    PhaseData activePower;
    PhaseData apparentPower;
    PhaseData reactivePower;
    PhaseData powerFactor;
    PhaseData voltageThd;
    PhaseData currentThd;

    double fundamentalVoltageRms;
    double fundamentalCurrentRms;

    int dominantHarmonicVoltageOrder;
    double dominantHarmonicVoltageRms;

    int dominantHarmonicCurrentOrder;
    double dominantHarmonicCurrentRms;

    double totalActivePower = 0.0;
    double totalApparentPower = 0.0;
    double totalReactivePower = 0.0;
    double totalPowerFactor = 0.0;
    double totalEnergyWh; // 누적 전력량

    // 1초 구간의 마지막 사이클에서 가져온 값들g
    double fundamentalVoltagePhase;
    double fundamentalCurrentPhase;
    double dominantHarmonicVoltagePhase;
    double dominantHarmonicCurrentPhase;

    SymmetricalComponents voltageSymmetricalComponents;
    SymmetricalComponents currentSymmetricalComponents;

    double residualVoltageRms = 0.0; // 1초 평균 잔류 전압
    double residualCurrentRms = 0.0; // 1초 평균 잔류 전류
    double nemaVoltageUnbalance = 0.0;
    double nemaCurrentUnbalance = 0.0;

    double voltageU0Unbalance = 0.0;
    double voltageU2Unbalance = 0.0;
    double currentU0Unbalance = 0.0;
    double currentU2Unbalance = 0.0;
};

Q_DECLARE_METATYPE(MeasuredData)
#endif // MEASURED_DATA_H
