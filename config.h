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
            static constexpr double Default = 10.0;
        };

        // 전압 관련 설정
        struct Amplitude {
            static constexpr double Min = -500.0;
            static constexpr double Max = 500.0;
            static constexpr double Default = 220.0;

        };

        // 전류 관련 설정
        struct Current {
            static constexpr double MinAmplitude = -500.0;
            static constexpr double MaxAmplitude = 500.0;
            static constexpr double DefaultAmplitude = 10.0;
            static constexpr int DefaultPhaseOffset = 0;
        };

        // 3상 관련 설정
        struct ThreePhase {
            // Voltage
            static constexpr double DefaultAmplitudeB = Source::Amplitude::Default;
            static constexpr double DefaultAmplitudeC = Source::Amplitude::Default;
            static constexpr double DefaultPhaseB_deg = -120.0;
            static constexpr double DefaultPhaseC_deg = 120.0;
            // Current
            static constexpr double DefaultCurrentAmplitudeB = Source::Current::DefaultAmplitude;
            static constexpr double DefaultCurrentAmplitudeC = Source::Current::DefaultAmplitude;
            static constexpr double DefaultCurrentPhaseB_deg = -120.0;
            static constexpr double DefaultCurrentPhaseC_deg = 120.0;
        };
    };

    // 샘플링 설정
    struct Sampling {
        static constexpr double DefaultSamplingCycles = 10.0;
        static constexpr int DefaultSamplesPerCycle = 20;
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

    // 고조파 설정
    struct Harmonics {
        static constexpr int DefaultOrder = 2;
        static constexpr double MaxMangnitude = 500;
        static constexpr double DefaultMagnitude = 0.0;
        static constexpr double DefaultPhase = 0.0;
    };

    // 수학 관련 상수
    struct Math {
        static constexpr double TwoPi = 2.0 * std::numbers::pi;
    };


}

enum class ChartDataType {
    VoltageA, VoltageB, VoltageC,
    CurrentA, CurrentB, CurrentC
};

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

    constexpr QPointF to_qpointf(const DataPoint& p, ChartDataType type) {
        const auto x = std::chrono::duration_cast<FpSeconds>(p.timestamp).count();
        double y = 0.0;

        switch(type) {
            case ChartDataType::VoltageA: y = p.voltage.a; break;
            case ChartDataType::VoltageB: y = p.voltage.b; break;
            case ChartDataType::VoltageC: y = p.voltage.c; break;
            case ChartDataType::CurrentA: y = p.current.a; break;
            case ChartDataType::CurrentB: y = p.current.b; break;
            case ChartDataType::CurrentC: y = p.current.c; break;
        }

        return QPointF(x, y);
    };
}

#endif // CONFIG_H
