#include "UIutils.h"

ScaleUnit UIutils::updateScaleUnit(double range)
{
    if(range < 1.0) return ScaleUnit::Milli;
    if(range >= 1000.0) return ScaleUnit::Kilo;
    return ScaleUnit::Base;
}

double UIutils::scaleValue(double value, ScaleUnit unit)
{
    switch(unit)
    {
    case ScaleUnit::Milli:
        return value * 1000.0;
    case ScaleUnit::Kilo:
        return value / 1000.0;
    default:
        return value;
    }
}

QString UIutils::formatValue(double value)
{
    QString formattedValue;

    if(value >= 100.0) {
        formattedValue = QString::number(value, 'f', 1);
    } else if(value >= 10.0) {
        formattedValue = QString::number(value, 'f', 2);
    } else if(value >= 1.0) {
        formattedValue = QString::number(value, 'f', 3);
    } else {
        formattedValue = QString::number(value, 'f', 4);
    }

    // 소수점 포함 최대 5글자 표시
    if(formattedValue.length() > 4 && formattedValue.contains('.')) {
        formattedValue = formattedValue.left(5);
    }

    return formattedValue;
}
