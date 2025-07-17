#ifndef CONFIG_H
#define CONFIG_H

#include <cmath>

namespace config {
    // 전압 설정
    constexpr double MinVoltage = -500.0;
    constexpr double MaxVoltage = 500.0;
    constexpr double DefaultVoltage = 220.0;

    // 주파수 설정
    constexpr double MinFrequency = 1.0;
    constexpr double MaxFrequency = 100.0;
    constexpr double DefaultFrequency = 1.0;

    // Simulation Engine 설정
    constexpr int DefaultDataSize = 100;
    constexpr int MinDataSize = 1;
    constexpr int MaxDataSize = 10000;
    constexpr int DefaultIntervalMs = 100;
    constexpr double MinIntervalSec = 0.001;

    // 그래프 설정
    constexpr double DefaultGraphWidthSec = 10.0;
    constexpr double MinGraphWidthSec = 0.01;
    constexpr double MaxGraphWidthSec = 300.0;

    // UI 설정
    constexpr int DialMin = 0;
    constexpr int DialMax = 359;

    // 샘플링 설정
    constexpr int DefaultSamplingCycles = 10;
    constexpr int DefaultSamplesPercle = 10;
    constexpr int MinSampligValue = 1;
    constexpr int maxSamplingValue = 1000;


    // AC 파형 설정
    constexpr double PI = M_PI;
}

#endif // CONFIG_H
