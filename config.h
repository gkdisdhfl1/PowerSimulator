#ifndef CONFIG_H
#define CONFIG_H

#include <cmath>
#include <QString>
#include <string_view>
#include <chrono>
#include <QPointF>
#include <data_point.h>

namespace config {
    // std::string_view를 QString으로 변환하는 헬퍼 함수
    inline QString sv_to_q(std::string_view sv) {
        return QString::fromUtf8(sv.data(), sv.size());
    }

    // Simulation 핵심 동작과 관련된 설정
    struct Simulation {
        static constexpr int DefaultDataSize = 1000;
        static constexpr int MinDataSize = 1;
        static constexpr int MaxDataSize = 100000;

    };

    // 데이터 source(파형)의 특성과 관련된 설정
    struct Source {
        // 주파수 관련 설정
        struct Frequency {
            static constexpr double Min = 0.0;
            static constexpr double Max = 100.0;
            static constexpr double Default = 1.0;
        };

        // 진폭 관련 설정
        struct Amplitude {
            static constexpr double Min = -500.0;
            static constexpr double Max = 500.0;
            static constexpr double Default = 220.0;

        };

        struct Current {
            static constexpr double MinAmplitude = -500.0;
            static constexpr double MaxAmplitude = 500.0;
            static constexpr double DefaultAmplitude = 10.0;
            static constexpr int DefaultPhaseOffset = 0;
        };

    };

    // 샘플링 설정
    struct Sampling {
        static constexpr double DefaultSamplingCycles = 1.0;
        static constexpr int DefaultSamplesPerCycle = 10;
        static constexpr int MinValue = 1;
        static constexpr int maxValue = 100;
        static constexpr double MaxSamplesPerSecond = 1000.0;
    };

    // 시간 비율 설정
    struct TimeScale {
        static constexpr double Min = 1.0;
        static constexpr double Max = 100.0;
        static constexpr double Default = 1.0;
    };


    // 그래프 시각적 표현 관련 상수
    struct View {
        // Y축 패딩 설정
        struct Padding {
            static constexpr double Ratio = 0.1;
            static constexpr double Min = 5.0;
        };

        // 그래프 폭 관련 설정
        struct GraphWidth {
            static constexpr double Default = 10.0;
            static constexpr double Min = 0.01;
            static constexpr double Max = 300.0;
        };

        // 사용자 상호작용 관련 상수
        struct Interaction {
            // 줌 팩터
            struct Zoom {
                static constexpr double FactorIn = 1.1;
                static constexpr double FactorOut = 0.9;
            };

            // 가장 가까운 점을 찾기 위한 픽셀 거리 임계값
            struct Proximity {
                static constexpr double Threshold = 20.0;
            };
        };
    };

    // 고조파 설정
    struct Harmonics {
        static constexpr int DefaultOrder = 2;
        static constexpr double MaxMangnitude = 500;
        static constexpr double DefaultMagnitude = 0.0;
        static constexpr double DefaultPhase = 0.0;
    };
}

namespace utils {
using FpSeconds = std::chrono::duration<double>;
using FpNanoseconds = std::chrono::duration<double, std::nano>;
using Nanoseconds = std::chrono::nanoseconds;

    constexpr double degreesToRadians(double degrees) {
        return degrees * (std::numbers::pi / 180.0);
    }

    constexpr double radiansToDegrees(double radians) {
        return radians * (180.0 / std::numbers::pi);
    }

    constexpr QPointF to_qpointf(const DataPoint& p) {
        const auto x = std::chrono::duration_cast<FpSeconds>(p.timestamp).count();
        return QPointF(x, p.voltage);
    };
}

#endif // CONFIG_H
