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
    QCOMPARE(results[1].phasor.real(), 0.0);
    QCOMPARE(results[1].phasor.imag(), 100.0);
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

QTEST_MAIN(TestAnalysisUtils)
#include "test_analysis_utils.moc"
