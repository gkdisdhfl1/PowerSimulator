#ifndef UIUTILS_H
#define UIUTILS_H

#include "UIconfig.h"
#include "shared_data_types.h"

class UIutils
{
public:
    static ScaleUnit updateScaleUnit(double range);
    static double scaleValue(double value, ScaleUnit unit);

    // double 값을 유효 숫자 4자리로 포맷팅하여 반환(예: 12.34, 1.234, 0.123)
    static QString formatValue(double value);

    // 고조파 리스트 <-> JSON 문자열 변환
    static QString harmonicListToJson(const HarmonicList& list);
    static HarmonicList jsonToHarmonicList(const QString& jsonStr);
};

#endif // UIUTILS_H
