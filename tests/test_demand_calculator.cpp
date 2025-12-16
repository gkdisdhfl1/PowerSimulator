#include <QtTest>
#include <QDateTime>
#include <complex>
#include "../demand_calculator.h"

class TestDemandCalculator : public QObject
{
    Q_OBJECT

private slots:
    // 1. DemandCalculator 초기 상태 및 값 업데이트 테스트
    void testProcessOneSecondData();

    void testSymmetricalComponentsBinding();

    void testNaNHandling();

    void testTimestampPolicy();

    void testFullMappingCoverage();
};

void TestDemandCalculator::testProcessOneSecondData()
{
    DemandCalculator calculator;
    const DemandData &demand = calculator.getDemandData(); // DemandCalculator의 내부 DemandData 참조

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
    QCOMPARE(demand.totalVoltageRms.a.min.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.b.max.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.b.min.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.c.max.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.c.min.value, 100.0);
    QVERIFY(demand.totalVoltageRms.a.max.timestamp.isValid());
    QVERIFY(demand.totalVoltageRms.a.min.timestamp.isValid());

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
    QCOMPARE(demand.totalVoltageRms.a.min.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.b.max.value, 100.0);
    QCOMPARE(demand.totalVoltageRms.b.min.value, 90.0);
    QCOMPARE(demand.totalVoltageRms.c.max.value, 105.0);
    QCOMPARE(demand.totalVoltageRms.c.min.value, 100.0);
    QVERIFY(demand.totalVoltageRms.a.max.timestamp.isValid());
    QVERIFY(demand.totalVoltageRms.a.min.timestamp.isValid());
}

void TestDemandCalculator::testSymmetricalComponentsBinding()
{
    DemandCalculator calculator;
    const DemandData &demand = calculator.getDemandData();

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

void TestDemandCalculator::testNaNHandling()
{
    DemandCalculator calculator;
    const DemandData &demand = calculator.getDemandData();

    OneSecondSummaryData data;
    data.totalVoltageRms = {std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN()};

    calculator.processOneSecondData(data);

    // NaN 값이 들어왔을 때 MinMaxTracker가 올바르게 처리하는지 검증
    // Nan 값은 무시되어야 함
    QCOMPARE(demand.totalVoltageRms.a.min.value, std::numeric_limits<double>::max());
    QCOMPARE(demand.totalVoltageRms.a.max.value, std::numeric_limits<double>::lowest());
}

void TestDemandCalculator::testTimestampPolicy()
{
    DemandCalculator calculator;
    const DemandData &demand = calculator.getDemandData();
    OneSecondSummaryData data;

    data.totalVoltageRms = {100.0, 100.0, 100.0};

    // 1. 첫 번째 입력
    calculator.processOneSecondData(data);
    QDateTime firstMaxTime = calculator.getDemandData().totalVoltageRms.a.max.timestamp;
    QDateTime firstMinTime = calculator.getDemandData().totalVoltageRms.a.min.timestamp;

    QVERIFY(firstMaxTime.isValid());
    QVERIFY(firstMinTime.isValid());

    QTest::qWait(100); // 타임스탬프 차이를 위해 잠시 대기

    // 2. 동일한 값 다시 입력
    calculator.processOneSecondData(data);
    QDateTime secondMaxTime = calculator.getDemandData().totalVoltageRms.a.max.timestamp;
    QDateTime secondMinTime = calculator.getDemandData().totalVoltageRms.a.min.timestamp;

    // 검증: 동일한 값이므로 타임스탬프는 변경되지 않아야 함
    QCOMPARE(firstMaxTime, secondMaxTime);
    QCOMPARE(firstMinTime, secondMinTime);

    QTest::qWait(100); // 타임스탬프 차이를 위해 잠시 대기

    // 3. 더 큰 값 입력 -> max 타임스탬프만 변경
    data.totalVoltageRms = {110.0, 110.0, 110.0};
    calculator.processOneSecondData(data);
    QDateTime thirdMaxTime = calculator.getDemandData().totalVoltageRms.a.max.timestamp;
    QDateTime thirdMinTime = calculator.getDemandData().totalVoltageRms.a.min.timestamp;
    QVERIFY(thirdMaxTime > secondMaxTime);
    QCOMPARE(thirdMinTime, secondMinTime);

    QTest::qWait(100); // 타임스탬프 차이를 위해 잠시 대기

    // 4. 더 작은 값 입력 -> min 타임스탬프만 변경
    data.totalVoltageRms = {90.0, 90.0, 90.0};
    calculator.processOneSecondData(data);
    QDateTime fourthMaxTime = calculator.getDemandData().totalVoltageRms.a.max.timestamp;
    QDateTime fourthMinTime = calculator.getDemandData().totalVoltageRms.a.min.timestamp;
    QCOMPARE(fourthMaxTime, thirdMaxTime);
    QVERIFY(fourthMinTime > thirdMinTime);
}

void TestDemandCalculator::testFullMappingCoverage()
{
    DemandCalculator calculator;
    const DemandData &demand = calculator.getDemandData();
    OneSecondSummaryData data;

    // 1. 모든 필드에 식별 가능한 고유 값 할당
    // (값이 서로 겹치지 않게 하여 잘못 매핑된 경우를 잡아냄)

    data.totalVoltageRms = {121.0, 125.0, 123.0};
    data.totalCurrentRms = {15.0, 14.0, 13.0};
    data.activePower = {3000.0, 2500.0, 2000.0};
    data.apparentPower = {3500.0, 3000.0, 2500.0};
    data.reactivePower = {2000.0, 1500.0, 1000.0};
    data.powerFactor = {0.85, 0.83, 0.80};
    data.voltageThd = {3.0, 2.5, 2.0};
    data.currentThd = {4.0, 3.5, 3.0};
    data.totalVoltageRms_ll = {210.0, 205.0, 207.5};
    data.voltageThd_ll = {2.8, 2.3, 2.1};
    
    data.fundamentalVoltage = {
        {1, 110.0, 0.0, std::complex<double>(110.0, 0.0)},
        {1, 112.0, -10.0, std::complex<double>(110.5, -19.2)},
        {1, 111.0, 5.0, std::complex<double>(110.2, 9.6)}
    };
    data.fundamentalVoltage_ll = {
        {1, 215.0, 0.0, std::complex<double>(215.0, 0.0)},
        {1, 217.0, -8.0, std::complex<double>(214.5, -30.3)},
        {1, 216.0, 4.0, std::complex<double>(215.2, 15.1)}
    };
    data.fundamentalCurrent = {
        {1, 16.0, 0.0, std::complex<double>(16.0, 0.0)},
        {1, 15.5, -5.0, std::complex<double>(15.4, -1.35)},
        {1, 15.75, 3.0, std::complex<double>(15.1, 0.79)}
    };

    data.frequency = 59.9;
    data.totalActivePower = 4000.0;
    data.totalApparentPower = 4500.0;
    data.totalReactivePower = 2500.0;
    data.totalPowerFactor = 0.88;

    // GenericPhaseSymmetricalComponents<SymmetricalComponent> 필드들
    data.voltageSymmetricalComponents.zero = {6.0, 25.0};
    data.voltageSymmetricalComponents.positive = {98.0, 0.0};
    data.voltageSymmetricalComponents.negative = {4.0, -25.0};
    
    data.currentSymmetricalComponents.zero = {2.0, 20.0};
    data.currentSymmetricalComponents.positive = {16.0, 0.0};
    data.currentSymmetricalComponents.negative = {1.0, -20.0};
    
    data.voltageSymmetricalComponents_ll.positive = {7.0, 15.0};
    data.voltageSymmetricalComponents_ll.negative = {3.0, -15.0};

    data.residualVoltageRms = 2.0;
    data.residualCurrentRms = 0.5;
    data.residualVoltageFundamental = 1.5;
    data.residualCurrentFundamental = 0.3;

    data.nemaVoltageUnbalance = 1.2;
    data.nemaCurrentUnbalance = 0.8;
    data.nemaVoltageUnbalance_ll = 1.0;

    data.voltageU0Unbalance = 0.6;
    data.voltageU2Unbalance = 0.4;
    data.currentU0Unbalance = 0.3;
    data.currentU2Unbalance = 0.2;

    // 2. 데이터 처리
    calculator.processOneSecondData(data);

    // 3. 각 필드가 올바르게 매핑되었는지 검증
    QCOMPARE(demand.totalVoltageRms.a.max.value, 121.0);
    QCOMPARE(demand.totalVoltageRms.b.max.value, 125.0);
    QCOMPARE(demand.totalVoltageRms.c.max.value, 123.0);
    QCOMPARE(demand.averageTotalVoltageRms.max.value, 123.0);

    QCOMPARE(demand.totalCurrentRms.a.max.value, 15.0);
    QCOMPARE(demand.totalCurrentRms.b.max.value, 14.0);
    QCOMPARE(demand.totalCurrentRms.c.max.value, 13.0);
    QCOMPARE(demand.averageTotalCurrentRms.max.value, 14.0);

    QCOMPARE(demand.activePower.a.max.value, 3000.0);
    QCOMPARE(demand.activePower.b.max.value, 2500.0);
    QCOMPARE(demand.activePower.c.max.value, 2000.0);

    QCOMPARE(demand.apparentPower.a.max.value, 3500.0);
    QCOMPARE(demand.apparentPower.b.max.value, 3000.0);
    QCOMPARE(demand.apparentPower.c.max.value, 2500.0);

    QCOMPARE(demand.reactivePower.a.max.value, 2000.0);
    QCOMPARE(demand.reactivePower.b.max.value, 1500.0);
    QCOMPARE(demand.reactivePower.c.max.value, 1000.0);

    QCOMPARE(demand.powerFactor.a.max.value, 0.85);
    QCOMPARE(demand.powerFactor.b.max.value, 0.83);
    QCOMPARE(demand.powerFactor.c.max.value, 0.80);

    QCOMPARE(demand.voltageThd.a.value, 3.0);
    QCOMPARE(demand.voltageThd.b.value, 2.5);
    QCOMPARE(demand.voltageThd.c.value, 2.0);

    QCOMPARE(demand.currentThd.a.value, 4.0);
    QCOMPARE(demand.currentThd.b.value, 3.5);
    QCOMPARE(demand.currentThd.c.value, 3.0);

    QCOMPARE(demand.totalVoltageRms_ll.ab.max.value, 210.0);
    QCOMPARE(demand.totalVoltageRms_ll.bc.max.value, 205.0);
    QCOMPARE(demand.totalVoltageRms_ll.ca.max.value, 207.5);
    QCOMPARE(demand.averageTotalVoltageRms_ll.max.value, 207.5);

    QCOMPARE(demand.voltageThd_ll.ab.value, 2.8);
    QCOMPARE(demand.voltageThd_ll.bc.value, 2.3);
    QCOMPARE(demand.voltageThd_ll.ca.value, 2.1);

    QCOMPARE(demand.fundamentalVoltageRMS.a.max.value, 110.0);
    QCOMPARE(demand.fundamentalVoltageRMS.b.max.value, 112.0);
    QCOMPARE(demand.fundamentalVoltageRMS.c.max.value, 111.0);
    QCOMPARE(demand.averageFundamentalVoltageRms.max.value, 111.0);

    QCOMPARE(demand.fundamentalVoltageRMS_ll.ab.max.value, 215.0);
    QCOMPARE(demand.fundamentalVoltageRMS_ll.bc.max.value, 217.0);
    QCOMPARE(demand.fundamentalVoltageRMS_ll.ca.max.value, 216.0);
    QCOMPARE(demand.averageFundamentalVoltageRms_ll.max.value, 216.0);

    QCOMPARE(demand.fundamentalCurrentRMS.a.max.value, 16.0);
    QCOMPARE(demand.fundamentalCurrentRMS.b.max.value, 15.5);
    QCOMPARE(demand.fundamentalCurrentRMS.c.max.value, 15.75);
    QCOMPARE(demand.averageFundamentalCurrentRms.max.value, 15.75);

    QCOMPARE(demand.frequency.max.value, 59.9);
    QCOMPARE(demand.totalActivePower.max.value, 4000.0);
    QCOMPARE(demand.totalApparentPower.max.value, 4500.0);
    QCOMPARE(demand.totalReactivePower.max.value, 2500.0);
    QCOMPARE(demand.totalPowerFactor.max.value, 0.88);
    
    QCOMPARE(demand.voltageSymmetricalComponents.zero.value, 6.0);
    QCOMPARE(demand.voltageSymmetricalComponents.positive.value, 98.0);
    QCOMPARE(demand.voltageSymmetricalComponents.negative.value, 4.0);

    QCOMPARE(demand.currentSymmetricalComponents.zero.value, 2.0);
    QCOMPARE(demand.currentSymmetricalComponents.positive.value, 16.0);
    QCOMPARE(demand.currentSymmetricalComponents.negative.value, 1.0);

    QCOMPARE(demand.voltageSymmetricalComponents_ll.positive.value, 7.0);
    QCOMPARE(demand.voltageSymmetricalComponents_ll.negative.value, 3.0);

    QCOMPARE(demand.voltageResidualRms.max.value, 2.0);
    QCOMPARE(demand.currentResidualRms.max.value, 0.5);
    QCOMPARE(demand.voltageResidualFundamental.max.value, 1.5);
    QCOMPARE(demand.currentResidualFundamental.max.value, 0.3);

    QCOMPARE(demand.nemaVoltageUnbalance.value, 1.2);
    QCOMPARE(demand.nemaCurrentUnbalance.value, 0.8);
    QCOMPARE(demand.nemaVoltageUnbalance_ll.value, 1.0);

    QCOMPARE(demand.voltageU0Unbalance.value, 0.6);
    QCOMPARE(demand.voltageU2Unbalance.value, 0.4);
    QCOMPARE(demand.currentU0Unbalance.value, 0.3);
    QCOMPARE(demand.currentU2Unbalance.value, 0.2);

}

QTEST_MAIN(TestDemandCalculator)
#include "test_demand_calculator.moc"