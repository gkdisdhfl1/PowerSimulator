#ifndef UICONFIG_H
#define UICONFIG_H

#include <QColor>
#include <array>

enum class ScaleUnit { Milli, Base, Kilo };

// 그래프 시각적 표현 관련 상수
struct View {
    // Y축 패딩 설정
    struct Padding {
        static constexpr double Ratio = 0.1;
        static constexpr double Min = 5.0;
    };
    // 그래프 폭 관련 설정
    struct GraphWidth {
        static constexpr double Default = 1.0;
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
    // harmonics, waveform y축 범위
    constexpr static const std::array<double, 18> RANGE_TABLE = {
        0.004, 0.008, 0.020, 0.040, 0.080,
        0.200, 0.400, 0.800,
        2.0, 4.0, 8.0, 20.0 , 40.0, 80.0,
        200.0, 400.0, 800.0, 2000.0
    };

    // 3상을 구분하는 색상
    struct PhaseColors {
        constexpr static std::array<QColor, 3> Voltage = {
            QColor(0, 0, 0),    // black
            QColor(255, 0, 0),  // red
            QColor(0, 0, 255)   // blue
        };
        constexpr static std::array<QColor, 3> Current = {
            QColor(0, 100, 0),    // dark green
            QColor(255, 165, 0),  // orange
            QColor(135, 206, 235)   // sky blue
        };
    };
};

#endif // UICONFIG_H
