#ifndef DATA_POINT_H
#define DATA_POINT_H

#include <chrono>

struct DataPoint {
    std::chrono::nanoseconds timestamp; // 경과 시간 (나노초)
    double voltage; // 전압
};

#endif // DATA_POINT_H
