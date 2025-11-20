#ifndef MIN_MAX_TRACKER_H
#define MIN_MAX_TRACKER_H

#include <QDateTime>
#include <limits>

template<typename T>
struct ValueWithTimestamp {
    T value = std::numeric_limits<T>::quiet_NaN();
    QDateTime timestamp;

    void update(const T& newValue, const QDateTime& newTimestamp, bool findMax) {
        if(std::isnan(value)) {
            value = newValue;
            timestamp = newTimestamp;
        } else if ((findMax && newValue > value) || (!findMax && newValue < value)) {
            value = newValue;
            timestamp = newTimestamp;
        }
    }
};

template <typename T>
struct MinMaxTracker {
    ValueWithTimestamp<T> min;
    ValueWithTimestamp<T> max;

    void update(const T& newValue, const QDateTime& newTimestamp) {
        min.update(newValue, newTimestamp, false); // 최소값 업데이트
        max.update(newValue, newTimestamp, true); // 최대값 업데이트
    }
};

#endif // MIN_MAX_TRACKER_H
