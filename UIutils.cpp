#include "UIutils.h"
#include <QJsonObject>
#include <QJsonArray>


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

QString UIutils::harmonicListToJson(const HarmonicList& list) {
    QJsonArray jsonArray;
    for(const auto& h : list) {
        QJsonObject obj;
        obj["order"] = h.order;
        obj["mag"] = h.magnitude;
        obj["phase"] = h.phase;
        jsonArray.append(obj);
    }
    QJsonDocument doc(jsonArray);
    return doc.toJson(QJsonDocument::Compact);
}

HarmonicList UIutils::jsonToHarmonicList(const QString& jsonStr)
{
    HarmonicList list;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if(!doc.isArray())
        return list;

    QJsonArray array = doc.array();
    for(const auto& val : std::as_const(array)) {
        if(val.isObject()) {
            QJsonObject obj = val.toObject();
            HarmonicComponent h;
            h.order = obj["order"].toInt();
            h.magnitude = obj["mag"].toDouble();
            h.phase = obj["phase"].toDouble();
            list.push_back(h);
        }
    }
    return list;
}
