#include <QtTest>
#include <QDateTime>
#include "../demand_calculator.h"

class TestDemandCalculator : public QObject
{
    Q_OBJECT

private slots:
    // 1. DemandCalculator 초기 상태 및 값 업데이트 테스트
    void testProcessOneSecondData()
    {
        DemandCalculator calculator;
        const DemandData& demand = calculator.getDemandData(); // DemandCalculator의 내부 DemandData 참조
        QDateTime currentTimestamp = QDateTime::currentDateTime();

        // 1.1 초기 상태 검증
        // 초기값은 MinMaxTracker 생성자에서 설정됨
        QCOMPARE(demand.totalVoltageRms.a.min.value, std::numeric_limits<double>::max());
        QCOMPARE(demand.totalVoltageRms.a.max.value, std::numeric_limits<double>::lowest());

        // 1.2. 첫 번째 데이터 (기준점)
        OneSecondSummaryData data1;
        data1.totalVoltageRms = {100.0, 100.0, 100.0};
        data1.totalCurrentRms = {10.0, 10.0, 10.0};
        data1.frequency = 60.0;
        data1.voltageThd = {1.0, 1.0, 1.0};

        calculator.processOneSecondData(data1);

        // 검증
        QCOMPARE(demand.totalVoltageRms.a.max.value, 100.0);
        QVERIFY(demand.totalVoltageRms.a.max.timestamp.isValid());

        OneSecondSummaryData data2;
        data2.totalVoltageRms = {110.0, 90.0, 105.0};
        data2.totalCurrentRms = {12.0, 9.0, 11.0};
        data2.frequency = 60.1;
        data2.voltageThd = {2.0, 0.5, 1.5};
        data2.totalActivePower = 2500.0;

        QTest::qWait(100); // 타임스탬프 차이를 위해 잠시 대기

        calculator.processOneSecondData(data2);

        // 검증
        QCOMPARE(demand.totalVoltageRms.a.max.value, 110.0);
        QCOMPARE(demand.totalVoltageRms.b.min.value, 90.0);
        QVERIFY(demand.totalVoltageRms.a.max.timestamp.isValid());
    }

    void testSymmetricalComponentsBinding()
    {
        DemandCalculator calculator;
        const DemandData& demand = calculator.getDemandData();

        OneSecondSummaryData data;
        data.voltageSymmetricalComponents.zero = {5.0, 30.0};
        data.voltageSymmetricalComponents.positive = {95.0, 0.0};
        data.voltageSymmetricalComponents.negative = {3.0, -30.0};

        calculator.processOneSecondData(data);

        // 검증
        QCOMPARE(demand.voltageSymmetricalComponents.zero.value, 5.0);
        QCOMPARE(demand.voltageSymmetricalComponents.positive.value, 95.0);
        QCOMPARE(demand.voltageSymmetricalComponents.negative.value, 3.0);
    }
};

QTEST_MAIN(TestDemandCalculator)
#include "test_demand_calculator.moc"