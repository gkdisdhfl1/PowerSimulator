#ifndef UIUTILS_H
#define UIUTILS_H

#include "UIconfig.h"

class UIutils
{
public:
    static ScaleUnit updateScaleUnit(double range);
    static double scaleValue(double value, ScaleUnit unit);

    // double 값을 유효 숫자 4자리로 포맷팅하여 반환(예: 12.34, 1.234, 0.123)
    static QString formatValue(double value);
};

#endif // UIUTILS_H
