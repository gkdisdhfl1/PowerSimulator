#ifndef DATA_POINT_H
#define DATA_POINT_H

#include <chrono>
#include "shared_data_types.h"

struct DataPoint {
    std::chrono::nanoseconds timestamp; // 경과 시간 (나노초)
    PhaseData voltage; // 전압
    PhaseData current; // 전류
};

#endif // DATA_POINT_H
