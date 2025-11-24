#include <iostream>
#include <cassert>
#include <cmath>
#include "min_max_tracker.h"

void test_min_tracker() {
    MinTracker<double> tracker;
    QDateTime t1 = QDateTime::currentDateTime();
    
    // Initial state should be max double
    assert(tracker.value == std::numeric_limits<double>::max());
    
    // Update with a value
    tracker.update(10.0, t1);
    assert(tracker.value == 10.0);
    assert(tracker.timestamp == t1);
    
    QDateTime t2 = QDateTime::currentDateTime();
    // Update with a larger value (should not change)
    tracker.update(20.0, t2);
    assert(tracker.value == 10.0);
    assert(tracker.timestamp == t1);
    
    // Update with a smaller value (should change)
    tracker.update(5.0, t2);
    assert(tracker.value == 5.0);
    assert(tracker.timestamp == t2);
    
    std::cout << "MinTracker test passed" << std::endl;
}

void test_max_tracker() {
    MaxTracker<double> tracker;
    QDateTime t1 = QDateTime::currentDateTime();
    
    // Initial state should be lowest double
    assert(tracker.value == std::numeric_limits<double>::lowest());
    
    // Update with a value
    tracker.update(10.0, t1);
    assert(tracker.value == 10.0);
    assert(tracker.timestamp == t1);
    
    QDateTime t2 = QDateTime::currentDateTime();
    // Update with a smaller value (should not change)
    tracker.update(5.0, t1);
    assert(tracker.value == 10.0);
    assert(tracker.timestamp == t1);
    
    // Update with a larger value (should change)
    tracker.update(20.0, t2);
    assert(tracker.value == 20.0);
    assert(tracker.timestamp == t2);
    
    std::cout << "MaxTracker test passed" << std::endl;
}

int main() {
    test_min_tracker();
    test_max_tracker();
    return 0;
}
