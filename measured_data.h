#ifndef MEASURED_DATA_H
#define MEASURED_DATA_H

#include <chrono>

// 단일 고조파 성분의 분석 결과를 담는 구조체
struct HarmonicAnalysisResult {
    int order;      // 차수
    double rms;     // RMS 크기
    double phase;   // 위상(라디안)
    double phasorX;  // cos 성분(실수)
    double phasorY;  // sin 성분(허수)
};

// 한 사이클 동안 연산 결과를 담는 구조체
struct MeasuredData {
    std::chrono::nanoseconds timestamp; // 사이클이 끝나는 시점의 타임스탬프
    double voltageRms; // 전압 RMS
    double currentRms; // 전류 RMS
    double activePower; // 유효 전력

    // 전압의 주파수 성분 분석 결과 (harmonics[0]: 기본파)
    std::vector<HarmonicAnalysisResult> voltageHarmonics;

    // 전압의 주파수 성분 분석 결과 (harmonics[0]: 기본파)
    std::vector<HarmonicAnalysisResult> currentarmonics;

    // DFT 연산 결과 (Phasor)를 저장할 변수 추가
    double voltagePhasorX;
    double voltagePhasorY;
    double currentPhasorX;
    double currentPhasorY;
};

#endif // MEASURED_DATA_H
