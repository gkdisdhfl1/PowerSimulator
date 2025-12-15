#ifndef UIUTILS_H
#define UIUTILS_H

#include "UIconfig.h"

class UiUtils
{
public:
    static ScaleUnit updateScaleUnit(double range);
    static double scaleValue(double value, ScaleUnit unit);

    // 소숫점 포함 4자리 출력
    static QString formatValue(double value);
};

#endif // UIUTILS_H
