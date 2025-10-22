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
};

// 1초 단위로 가공된 분석 데이터를 담는 구조체
struct OneSecondSummaryData {
    PhaseData totalVoltageRms;
    PhaseData totalCurrentRms;
    PhaseData activePower;

    double fundamentalVoltageRms;
    double fundamentalCurrentRms;

    int dominantHarmonicVoltageOrder;
    double dominantHarmonicVoltageRms;

    int dominantHarmonicCurrentOrder;
    double dominantHarmonicCurrentRms;

    double totalEnergyWh; // 누적 전력량

    // 1초 구간의 마지막 사이클에서 가져온 값들
    double fundamentalVoltagePhase;
    double fundamentalCurrentPhase;
    double dominantHarmonicVoltagePhase;
    double dominantHarmonicCurrentPhase;
};

// 추가 계측 항목 분석 데이터를 담는 구조체
struct AdditionalMetricsData
{
    // Residual
    double residualVoltageRms = 0.0;
    double residualCurrentRms = 0.0;

    // THD
    PhaseData voltageThd;
    PhaseData currentThd;

    // 피상전력
    PhaseData apparentPower;

    // 무효 전력
    PhaseData reactivePower;

    // 역률
    PhaseData powerFactor;
};

Q_DECLARE_METATYPE(MeasuredData)
#endif // MEASURED_DATA_H
