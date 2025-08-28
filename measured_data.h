#ifndef MEASURED_DATA_H
#define MEASURED_DATA_H

#include <chrono>

// 한 사이클 동안 연산 결과를 담는 구조체
struct MeasuredData {
    std::chrono::nanoseconds timestamp; // 사이클이 끝나는 시점의 타임스탬프
    double voltageRms; // 전압 RMS
    double currentRms; // 전류 RMS
    double activePower; // 유효 전력

    // DFT 연산 결과 (Phasor)를 저장할 변수 추가
    double voltagePhasorX;
    double voltagePhasorY;
    double currentPhasorX;
    double currentPhasorY;
};

#endif // MEASURED_DATA_H
