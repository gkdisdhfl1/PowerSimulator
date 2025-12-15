#ifndef UICONFIG_H
#define UICONFIG_H

#include <QColor>
#include <array>

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

#endif // UICONFIG_H
