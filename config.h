#ifndef CONFIG_H
#define CONFIG_H

namespace config {
    // 전압 설정
    constexpr double MinVoltage = -500.0;
    constexpr double MaxVoltage = 500.0;
    constexpr double DefaultVoltage = 220.0;

    // Simulation Engine 설정
    constexpr int DefaultDataSize = 100;
    constexpr int MinDataSize = 1;
    constexpr int MaxDataSize = 10000;
    constexpr int DefaultIntervalMs = 100;
    constexpr double MinIntervalSec = 0.01;

    // 그래프 설정
    constexpr double DefaultGraphWidthSec = 10.0;
    constexpr double MinGraphWidthSec = 1.0;
    constexpr double MaxGraphWidthSec = 300.0;

    // UI 설정
    constexpr int DialMin = 0;
    constexpr int DialMax = 359;

}

#endif // CONFIG_H
