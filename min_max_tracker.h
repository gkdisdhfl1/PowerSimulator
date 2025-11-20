#ifndef MIN_MAX_TRACKER_H
#define MIN_MAX_TRACKER_H

#include <QDateTime>
#include <limits>

template<typename T>
struct ValueWithTimestamp {
    T value;
    QDateTime timestamp;

    ValueWithTimestamp(T v) : value(v) {}
    ValueWithTimestamp() : value(std::numeric_limits<T>::quiet_NaN()) {}
};

// 최소값 추적
template <typename T>
struct MinTracker : public ValueWithTimestamp<T> {
    MinTracker() : ValueWithTimestamp<T>(std::numeric_limits<T>::max()) {}

    void update(const T& newValue, const QDateTime& newTimestamp) {
        if(newValue < this->value) {
            this->value = newValue;
            this->timestamp = newTimestamp;
        }
    }
};

// 최대값 추적
template <typename T>
struct MaxTracker : public ValueWithTimestamp<T> {
    MaxTracker() : ValueWithTimestamp<T>(std::numeric_limits<T>::lowest()) {}

    void update(const T& newValue, const QDateTime& newTimestamp) {
        if(newValue > this->value) {
            this->value = newValue;
            this->timestamp = newTimestamp;
        }
    }
};

template <typename T>
struct MinMaxTracker {
    MinTracker<T> min;
    MaxTracker<T> max;

    void update(const T& newValue, const QDateTime& newTimestamp) {
        min.update(newValue, newTimestamp);
        max.update(newValue, newTimestamp);
    }
};

#endif // MIN_MAX_TRACKER_H
