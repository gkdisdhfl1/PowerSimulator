#include <QtTest>
#include "../frequency_tracker.h"
#include "../simulation_engine.h"

class TestFrequencyTracker : public QObject
{
    Q_OBJECT

private:
    // 특정 시간 t에서의 위상을 계산하여 MeasuredData 생성
    MeasuredData createData(double frequencyHz, double timeSec);

private slots:
    void testInitialState();
    void testCoarseSearchTransition();
    void testFllToPllTransition();
    void testPLLTracking();
};

MeasuredData TestFrequencyTracker::createData(double frequencyHz, double timeSec)
{
    MeasuredData data;
    double phase = 2.0 * std::numbers::pi * frequencyHz * timeSec;
    // -pi ~ pi 정규화
    while(phase > std::numbers::pi) phase -= 2.0 * std::numbers::pi;
    while(phase <= -std::numbers::pi) phase += 2.0 * std::numbers::pi;

    data.fundamentalVoltage.a.rms = 100.0;
    data.fundamentalVoltage.a.phase = phase;
    data.fundamentalVoltage.a.order = 1;
    return data;
}

void TestFrequencyTracker::testInitialState()
{
    SimulationEngine engine; // 실제 엔진 생성
    FrequencyTracker tracker(&engine);

    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::Idle);

    tracker.startTracking();
    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::Coarse);

    tracker.stopTracking();
    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::Idle);
}

void TestFrequencyTracker::testCoarseSearchTransition()
{
    SimulationEngine engine;
    // 엔진 설정 (필수값)
    engine.m_samplingCycles.setValue(60.0);
    engine.m_samplesPerCycle.setValue(16); // 16 samples per cycle

    FrequencyTracker tracker(&engine);
    tracker.startTracking(); // Coarse 상태 진입

    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::Coarse);

    // Coarse Search에 필요한 데이터 주입
    // 60Hz Sine Wave 생성
    const double targetFrequency = 60.0;
    const double sampleRate = 60.0 * 16.0; // 960Hz
    const int samplesNeeded = 500;

    QSignalSpy spy(&tracker, &FrequencyTracker::samplingCyclesUpdated);

    for(int i = 0; i < samplesNeeded; ++i) {
        DataPoint dp;
        double t = i / sampleRate;
        dp.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(t));
        dp.voltage.a = 100.0 * std::sin(2.0 * std::numbers::pi * targetFrequency * t);

        // Coarse Search는 latestDataPoint만 사용
        tracker.process(dp, {}, {});

        if(tracker.currentState() == FrequencyTracker::TrackingState::FLL_Acquisition) {
            break; // 전환 성공
        }
    }

    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::FLL_Acquisition);
    QCOMPARE(spy.count(), 1); // 주파수 변경 신호가 한 번 발생했는지 확인

    // 추정된 주파수가 60Hz 근처인지 확인
    double estimatedFrequency = spy.takeFirst().at(0).toDouble();
    QVERIFY(std::abs(estimatedFrequency - 60.0) < 5.0); // 5Hz 이내 오차 허용
}

void TestFrequencyTracker::testFllToPllTransition()
{
    SimulationEngine engine;
    engine.m_samplingCycles.setValue(60.0);
    engine.m_samplesPerCycle.setValue(16);
    engine.recalculateCaptureInterval();

    FrequencyTracker tracker(&engine);
    QSignalSpy spy(&tracker, &FrequencyTracker::samplingCyclesUpdated);

    // 1. Coarse Search 진행 (60Hz 입력 -> FLL 진입)
    tracker.startTracking();
    const double targetFrequency = 60.0;
    const double sampleRate = 60.0 * 16.0; // 960Hz

    for(int i = 0; i < 600; ++i) {
        DataPoint dp;
        double t = i / sampleRate;
        dp.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(t));
        dp.voltage.a = 100.0 * std::sin(2.0 * std::numbers::pi * targetFrequency * t);

        tracker.process(dp, {}, {});

        if(tracker.currentState() == FrequencyTracker::TrackingState::FLL_Acquisition) {
            break;
        }
    }
    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::FLL_Acquisition);
    spy.clear(); // Coarse Search 결과 지움

    // 2. FLL 진행 (61Hz, 엔진: 60Hz에서 시작)
    // 60Hz 샘플링 기준으로 61Hz 신호를 샘플링하면 위상이 매 추기마다 조금씩 빨라짐
    double currentFrequency = 60.0; // 현재 엔진의 샘플링 주파수
    double time = 0.0;
    double inputFrequency = 61.0; // 실제 신호 주파수

    // FLL 수렴 테스트 (최대 200 사이클)
    for(int i = 0; i < 200; ++i) {
        double cycleDuration = 1.0 / currentFrequency;
        time += cycleDuration;

        MeasuredData md = createData(inputFrequency, time);
        std::vector<DataPoint> dummyBuffer(16); // 사이즈 조건 만족용

        tracker.process({}, md, dummyBuffer);

        // 시그널 확인
        if(spy.count() > 0) {
            currentFrequency = spy.takeLast().at(0).toDouble();
            // 엔진 설정도 업데이트해줘야 다음 계산이 맞음
            engine.m_samplingCycles.setValue(currentFrequency);
            engine.recalculateCaptureInterval();
        }
    }

    // 61Hz 근처로 수렴했는지 확인
    QVERIFY(std::abs(currentFrequency - 61.0) < 0.5);; // 0.5Hz 이내 오차 허용
    
    // 3. PLL 진입 확인 (Lock 유도)
    // 강제로 입력 주파수를 추적된 주파수와 일치시켜 오차를 0으로 만듦
    inputFrequency = currentFrequency;

    // Lock Count 채우기 (MinLockCount = 10)
    for(int i = 0; i < 20; ++i) {
        double cycleDuration = 1.0 / currentFrequency;
        time += cycleDuration;
        MeasuredData md = createData(inputFrequency, time);
        std::vector<DataPoint> dummyBuffer(16);
        tracker.process({}, md, dummyBuffer);

        if(spy.count() > 0) {
            currentFrequency = spy.takeLast().at(0).toDouble();
            engine.m_samplingCycles.setValue(currentFrequency);
            engine.recalculateCaptureInterval();
            inputFrequency = currentFrequency;
        }

        if(tracker.currentState() == FrequencyTracker::TrackingState::FineTune) break;
    }

    // PLL 상태 진입 확인
    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::FineTune);
}

void TestFrequencyTracker::testPLLTracking()
{
    SimulationEngine engine;
    engine.m_samplingCycles.setValue(60.0);
    engine.m_samplesPerCycle.setValue(16);
    engine.recalculateCaptureInterval();

    FrequencyTracker tracker(&engine);
    QSignalSpy spy(&tracker, &FrequencyTracker::samplingCyclesUpdated);
    std::vector<DataPoint> dummy(16);

    // 1. PLL 상태 강제 진입
    tracker.startTracking();
    const double targetFrequency = 60.0;
    const double sampleRate = 60.0 * 16.0;

    // Coarse Pass
    for(int i = 0; i < 600; ++i) {
        DataPoint dp;
        double t = i / sampleRate;
        dp.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(t));
        dp.voltage.a = 100.0 * std::sin(2.0 * std::numbers::pi * targetFrequency * t);

        tracker.process(dp, {}, {});

        if(tracker.currentState() == FrequencyTracker::TrackingState::FLL_Acquisition) break;
    }

    // FLL Pass
    // 1. 첫 번째 데이터: 위상 0.1 (초기화용)
    MeasuredData md_init;
    md_init.fundamentalVoltage.a = {.order = 1, .rms = 100.0, .phase = 0.1, .phasor = {}};
    tracker.process({}, md_init, dummy);

    // 2. 이후 데이터: 위상 0.1 (고정) -> 위상차 0 -> 주파수 오차 0
    for(int i = 0; i < 50; ++i) {
        MeasuredData md;
        md.fundamentalVoltage.a = {.order = 1, .rms = 100.0, .phase = 0.1, .phasor = {}}; // 위상 고정
        tracker.process({}, md, dummy);

        if(tracker.currentState() == FrequencyTracker::TrackingState::FineTune) break;
    }
    QCOMPARE(tracker.currentState(), FrequencyTracker::TrackingState::FineTune);

    // 2. PLL 동작 검증 (위상차 주입)
    // 위상을 0.1 라디안 틀어서 주입
    double injectedPhase = 0.1;

    // 초기 samplingCycles 값 저장
    double initialCycles = engine.m_samplingCycles.value();
    spy.clear();

    // 한 번 실행
    MeasuredData md;
    md.fundamentalVoltage.a.rms = 100.0;
    md.fundamentalVoltage.a.phase = injectedPhase;
    md.fundamentalVoltage.a.order = 1;
    tracker.process({}, md, dummy);
    tracker.process({}, md, dummy);

    // 검증: 위상차가 양수(+)이면, 트래커는 이를 줄이기 위해 주파수를 높여야함.
    QVERIFY(spy.count() > 0);
    double newCycles = spy.takeLast().at(0).toDouble();

    // 위상 에러가 양수 -> feedback이 양수 -> cycles 증가
    QVERIFY(newCycles > initialCycles);
}

QTEST_MAIN(TestFrequencyTracker)
#include "test_frequency_tracker.moc"
