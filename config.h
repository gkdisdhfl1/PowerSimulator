#ifndef CONFIG_H
#define CONFIG_H

#include <cmath>
#include <QString>
#include <string_view>

namespace config {
    // std::string_view를 QString으로 변환하는 헬퍼 함수
inline QString sv_to_q(std::string_view sv) {
    return QString::fromUtf8(sv.data(), sv.size());
    }

    // 진폭 설정
    struct Amplitude {
        static constexpr double Min = -500.0;
        static constexpr double Max = 500.0;
        static constexpr double Default = 220.0;

    };

    // 주파수 설정
    struct Frequency {
        static constexpr double Min = 1.0;
        static constexpr double Max = 100.0;
        static constexpr double Default = 1.0;
    };

    struct Simulation {
        // Simulation Engine 설정
        static constexpr int DefaultDataSize = 100;
        static constexpr int MinDataSize = 1;
        static constexpr int MaxDataSize = 10000;
        static constexpr double DefaultSamplingCycles = 1.0;
        static constexpr int DefaultSamplesPerCycle = 10;
        // static constexpr int DefaultIntervalMs = 100;
        // static constexpr double MinIntervalSec = 0.001;
    };

    // 그래프 설정
    struct GraphWidthSec {
        static constexpr double Default = 10.0;
        static constexpr double Min = 0.01;
        static constexpr double Max = 300.0;
    };


    // 샘플링 설정
    struct Sampling {
        static constexpr double DefaultSamplingCycles = 10;
        static constexpr int DefaultSamplesPerCycle = 10;
        static constexpr int MinValue = 1;
        static constexpr int maxValue = 1000;
    };

}

namespace utils {
    constexpr double degreesToRadians(double degrees) {
        return degrees * (std::numbers::pi / 180.0);
    }
}

#endif // CONFIG_H
