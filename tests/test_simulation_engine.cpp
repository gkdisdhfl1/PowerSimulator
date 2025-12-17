#include <QtTest>
#include "../simulation_engine.h"
#include "frequency_tracker.h"

class TestSimulationEngine : public QObject
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testStartStop();
    void testCaptureData();
    void testMaxDataSize();
    void testMeasuredDataUpdate();
};

void TestSimulationEngine::testInitialState()
{
    SimulationEngine engine;
    QVERIFY(!engine.isRunning());
    QCOMPARE(engine.getDataSize(), 0);
}

void TestSimulationEngine::testStartStop()
{
    SimulationEngine engine;
    QSignalSpy spy(&engine, &SimulationEngine::runningStateChanged);

    engine.start();
    QVERIFY(engine.isRunning());
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.takeFirst().at(0).toBool()); // true

    engine.stop();
    QVERIFY(!engine.isRunning());
    QCOMPARE(spy.count(), 1);
    QVERIFY(!spy.takeFirst().at(0).toBool()); // false
}

void TestSimulationEngine::testCaptureData()
{
    SimulationEngine engine;

    // 설정
    engine.m_amplitude.setValue(100.0);
    engine.m_frequency.setValue(60.0);

    QSignalSpy spy(&engine, &SimulationEngine::dataUpdated);

    // captureData 호출
    QMetaObject::invokeMethod(&engine, "captureData");

    QVERIFY(spy.count() > 0); // 데이터 업데이트 시그널 발생 확인

    // 데이터 검증
    auto args = spy.takeFirst();
    auto dataDeque = args.at(0).value<std::deque<DataPoint>>();
    QVERIFY(!dataDeque.empty());

    DataPoint dp = dataDeque.back();

    // 시뮬레이션 타임 0에서의 값 확인
    QVERIFY(std::abs(dp.voltage.a) < 0.001);
}

void TestSimulationEngine::testMaxDataSize()
{
    SimulationEngine engine;
    int maxSize = 10;
    engine.m_maxDataSize.setValue(maxSize);

    // maxSize보다 많은 데이터 생성
    for(int i = 0; i < maxSize + 5; ++i) {
        QMetaObject::invokeMethod(&engine, "captureData");
    }

    QCOMPARE(engine.getDataSize(), maxSize);
}

void TestSimulationEngine::testMeasuredDataUpdate()
{
    SimulationEngine engine;

    engine.m_samplingCycles.setValue(1.0);
    engine.m_samplesPerCycle.setValue(10);

    QSignalSpy spy(&engine, &SimulationEngine::measuredDataUpdated);

    // 1주기 분량(10개) 데이터 생성
    for(int i = 0; i < 10; ++i) {
        QMetaObject::invokeMethod(&engine, "captureData");
    }

    // 1주기가 끝나면 measuredDataUpdated 시그널이 발생해야 함
    QVERIFY(spy.count() > 0);
}

QTEST_MAIN(TestSimulationEngine)
#include "test_simulation_engine.moc"
