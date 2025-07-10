#ifndef DATA_POINT_H
#define DATA_POINT_H

#include <QtGlobal> // qint64 타입을 위해

struct DataPoint {
    qint64 timestampMs; // 경과 시간 (밀리초)
    double voltage; // 전압
};

#endif // DATA_POINT_H
