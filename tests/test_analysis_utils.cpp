#include <QtTest/QtTest>
#include "../analysis_utils.h"

// QTest 메인 함수 생성을 위한 매크로 사용
class TestAnalysisUtils : public QObject
{
    Q_OBJECT

private slots:
    // 1. RMS 계산 테스트 (DC) 신호
    void testCalculateTotalRms_DC();

    // 2. FFT 스펙트럼 계산 테스트 (sin wave)
    void testCalculateSpectrum_sineWave();

    // 3. 복소수 Phasor -> HarmonicAnalysisResult 변환 테스트
    // convertSpectrumToHarmonics를 통해 간접 테스트
    void testConvertSpectrumToHarmonics();

    // 4. 대칭 성분 테스트
    // Balanced System -> Positive Sequence만 존재해야 함
    void testSymmetricalComponents_Balanced();

    // 5. 복합 파형 (Fundamental + Harmonic) FFT 테스트
    void testComplexWaveformFFT();

    // 6. 유효 전력 위상차 테스트
    void testActivePowerPhaseShift();
};

void TestAnalysisUtils::testCalculateTotalRms_DC()
{
    std::vector<DataPoint> samples(100);
    // DC 10V 신호 생성
    for(auto& p : samples) {
        p.voltage.a = 10.0;
        p.timestamp = std::chrono::nanoseconds(0);
    }

    PhaseData rms = AnalysisUtils::calculateTotalRms(samples, AnalysisUtils::DataType::Voltage);

    // 오차 범위 0.001 내에서 10.0과 같은지 확인
    QCOMPARE_LE(std::abs(rms.a - 10.0), 0.001);
}

void TestAnalysisUtils::testCalculateSpectrum_sineWave()
{
    const int N = 1024;
    const double sampleRate = 1024.0;
    const double frequency = 60.0;
    const double amplitude = 100.0;

    std::vector<DataPoint> samples;
    samples.reserve(N);

    for(int i{0}; i < N; ++i) {
        double t = i / sampleRate;
        double val = amplitude * std::sin(2.0 * std::numbers::pi * frequency * t);
        DataPoint p;
        p.voltage.a = val;
        samples.push_back(p);
    }

    // FFT 실행 (Window 미사용)
    auto result = AnalysisUtils::calculateSpectrum(samples, AnalysisUtils::DataType::Voltage, 0, false);
    QVERIFY(result.has_value()); // 참인지 거짓인지 확인

    const auto& spectrum = result.value();

    // 60Hz 빈 인덱스 = 60 * 1024 / 1024 = 60
    int targetIdx = 60;

    // Spectrum 크기는 RMS 값이 나와야 함 (createHarmonicResult 등 고려)
    // Amplitude 100 -> RMS 70.71, rms = amp / sqrt(2.0)
    double expectedRms = amplitude / std::sqrt(2.0);
    double actualRms = std::abs(spectrum[targetIdx]); // std::abs(complex) = magnitude

    // RMS 값이 제대로 나오는지 확인 (오차 0.01)
    QVERIFY(std::abs(actualRms - expectedRms) < 0.01);
}

void TestAnalysisUtils::testConvertSpectrumToHarmonics()
{
    // 가상의 스펙트럼
    // index 0: DC (10V)
    // index 1: Fundamental (RMS 100V, phase 90deg) -> 0 + 100j
    AnalysisUtils::Spectrum spec(2);
    spec[0] = {10.0, 0.0};
    spec[1] = {0.0, 100.0};

    auto results = AnalysisUtils::convertSpectrumToHarmonics(spec);

    // DC 확인
    QCOMPARE(results[0].order, 0);
    QCOMPARE(results[0].rms, 10.0);
    QCOMPARE(results[0].phase, 0.0);

    // Fundamental 확인
    QCOMPARE(results[1].order, 1);
    QCOMPARE(results[1].rms, 100.0);
    // Phase: 90도 (pi / 2)
    QCOMPARE_LE(std::abs(results[1].phase - (std::numbers::pi / 2.0)), 0.001);
    // Phasor check
    QVERIFY(std::abs(results[1].phasor.real() - 0.0) < 0.001);
    QCOMPARE_LE(std::abs(results[1].phasor.imag() - 100.0), 0.001);
}

void TestAnalysisUtils::testSymmetricalComponents_Balanced()
{
    HarmonicAnalysisResult va, vb, vc;
    double mag = 100.0;

    // Va: 100 /_ 0
    va.phasor = std::polar(mag, 0.0);
    // Vb: 100 /_ -120 (-2PI/3)
    vb.phasor = std::polar(mag, -2.0 * std::numbers::pi / 3.0);
    // Vc: 100 /_ 120 (2PI/3)
    vc.phasor = std::polar(mag, 2.0 * std::numbers::pi / 3.0);

    SymmetricalComponents sym = AnalysisUtils::calculateSymmetricalComponents(va, vb, vc);

    // Positive Sequence: 100
    QCOMPARE_LE(std::abs(sym.positive.magnitude - 100.0), 0.001);
    // Zero Sequence: 0
    QCOMPARE_LE(std::abs(sym.zero.magnitude), 0.001);
    // Negative Sequence: 0
    QCOMPARE_LE(std::abs(sym.negative.magnitude), 0.001);
}

void TestAnalysisUtils::testComplexWaveformFFT()
{
    const int N = 1024;
    const double fs = 1024.0; // 샘플링 주파수

    // Signal: 100V, 60Hz + 20V, 180Hz (3rd) + 10V, 300Hz (5th)
    std::vector<DataPoint> samples;
    samples.reserve(N);
    for(int i{0}; i < N; ++i) {
        double t = i / fs;
        double val = 100.0 * std::sin(2.0 * std::numbers::pi * 60.0 * t) +
                     20.0 * std::sin(2.0 * std::numbers::pi * 180.0 * t) +
                     10.0 * std::sin(2.0 * std::numbers::pi * 300.0 * t);
        DataPoint p;
        p.voltage.a = val;
        samples.push_back(p);
    }

    auto result = AnalysisUtils::calculateSpectrum(samples, AnalysisUtils::DataType::Voltage, 0, false);
    QVERIFY(result.has_value());
    const auto& spectrum = result.value();

    // 60Hz 성분 확인 (Fund)
    double rms1 = std::abs(spectrum[60]);
    QVERIFY(std::abs(rms1 - (100.0 / std::sqrt(2.0))) < 0.001);

    // 180Hz 성분 확인 (3rd)
    double rms3 = std::abs(spectrum[180]);
    QVERIFY(std::abs(rms3 - (20.0 / std::sqrt(2.0))) < 0.001);

    // 300Hz 성분 확인 (5th)
    double rms5 = std::abs(spectrum[300]);
    QVERIFY(std::abs(rms5 - (10.0 / std::sqrt(2.0))) < 0.001);
}

void TestAnalysisUtils::testActivePowerPhaseShift()
{
    const int N = 100;
    std::vector<DataPoint> samples;
    double v_mag = 100.0; // Voltage amplitude
    double i_mag = 10.0;  // Current amplitude
    
    // Case 1: 0도 위상차 (Resistive) -> p = V * I
    // Case 2: 60도 위상차 -> p = V * I * cos(60) = VI * 0.5
    // Case 3: 90도 위상차 (Inductive) -> p = 0

    auto createSamples = [&](double phaseDiffRad) {
        std::vector<DataPoint> s;
        for(int i{0}; i < N; ++i) {
            double t = i / 100.0; // dummy time
            double v = v_mag * std::sin(2.0 * std::numbers::pi * t);
            double c = i_mag * std::sin(2.0 * std::numbers::pi * t - phaseDiffRad); // Current lags Voltage
            DataPoint p;
            p.voltage.a = v;
            p.current.a = c;
            s.push_back(p);
        }
        return s;
    };

    // 이론적 p = Vrms * Irms * cos(theta)
    // Vrms = 100/sqrt(2), Irms = 10/sqrt(2)
    // Vrms * Irms = (100*10)/2 = 500

    // Case 1: 0도
    auto s1 = createSamples(0.0);
    PhaseData p1 = AnalysisUtils::calculateActivePower(s1);

    QVERIFY(std::abs(p1.a - 500.0) < 0.001);

    // Case 2: 60도 (pi/3) -> cos(60) = 0.5 -> p = 250
    auto s2 = createSamples(std::numbers::pi / 3.0);
    PhaseData p2 = AnalysisUtils::calculateActivePower(s2);
    QVERIFY(std::abs(p2.a - 250.0) < 0.001);

    // Case 3: 90도 (pi/2) -> cos(90) = 0 -> p = 0
    auto s3 = createSamples(std::numbers::pi / 2.0);
    PhaseData p3 = AnalysisUtils::calculateActivePower(s3);
    QVERIFY(std::abs(p3.a) < 0.001);
}

QTEST_MAIN(TestAnalysisUtils)
#include "test_analysis_utils.moc"
