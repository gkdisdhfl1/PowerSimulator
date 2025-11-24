#include <QTest>
#include <QObject>
#include <QDebug>
#include "min_max_tracker.h"

class TestMinMaxTracker : public QObject
{
    Q_OBJECT
private slots: // private slot은 자동으로 테스트 케이스가 됨
    void test_min_tracker();
    void test_max_tracker();
};

void TestMinMaxTracker::test_min_tracker() {
    MinTracker<double> tracker;
    QDateTime t1 = QDateTime::currentDateTime();

    qDebug() << "MinTracker t1 : " << t1.toString("yyyy-MM-dd HH:mm:ss");

    // Initial state should be max double
    QCOMPARE(tracker.value , std::numeric_limits<double>::max());
    
    // Update with a value
    tracker.update(10.0, t1);
    QCOMPARE(tracker.value, 10.0);
    QCOMPARE(tracker.timestamp, t1);
    
    QDateTime t2 = t1.addMSecs(100); // 100ms 뒤 시간
    qDebug() << "MinTracker t2 : " << t2.toString("yyyy-MM-dd HH:mm:ss");
    // Update with a larger value (should not change)
    tracker.update(20.0, t2);
    QCOMPARE(tracker.value, 10.0);
    QCOMPARE(tracker.timestamp, t1);
    
    // Update with a smaller value (should change)
    tracker.update(5.0, t2);
    QCOMPARE(tracker.value, 5.0);
    QCOMPARE(tracker.timestamp, t2);
    
    qDebug() << "MinTracker test passed";
}

void TestMinMaxTracker::test_max_tracker() {
    MaxTracker<double> tracker;
    QDateTime t1 = QDateTime::currentDateTime();
    qDebug() << "MaxTracker t1 : " << t1.toString("yyyy-MM-dd HH:mm:ss");
    
    // Initial state should be lowest double
    QCOMPARE(tracker.value , std::numeric_limits<double>::lowest());
    
    // Update with a value
    tracker.update(10.0, t1);
    QCOMPARE(tracker.value, 10.0);
    QCOMPARE(tracker.timestamp, t1);
    
    QDateTime t2 = t1.addMSecs(100); // 100ms 뒤 시간
    qDebug() << "MaxTracker t2 : " << t2.toString("yyyy-MM-dd HH:mm:ss");
    // Update with a smaller value (should not change)
    tracker.update(5.0, t1);
    QCOMPARE(tracker.value, 10.0);
    QCOMPARE(tracker.timestamp, t1);
    
    // Update with a larger value (should change)
    tracker.update(20.0, t2);
    QCOMPARE(tracker.value, 20.0);
    QCOMPARE(tracker.timestamp, t2);
    
    qDebug() << "MaxTracker test passed";
}

// main 함수 자동 생성 매크로
QTEST_MAIN(TestMinMaxTracker)
#include "test_min_max.moc"
