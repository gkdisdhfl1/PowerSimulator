#ifndef ANALYSIS_UTILS_H
#define ANALYSIS_UTILS_H

#include "data_point.h"
#include "measured_data.h"
#include "kiss_fftr.h"
#include <cmath>
#include <complex>
#include <expected>
#include <QString>
#include <map>


class AnalysisUtils {
public:
    enum class SpectrumError {
        InvalidInput,       // N=0 or N = odd
        AllocationFailed    // kiss_fftr_alloc 실패
    };
    enum class WaveGenerateError {
        SpectrumCalculationFailed, // 스펙트럼 분석 자체 실패
        NoSignificantFound         // 스펙트럼은 있으나 유의미한 피크 없음
    };

    enum class DataType { Voltage, Current };

    inline static QString toQString(SpectrumError error) {
        switch (error) {
        case SpectrumError::InvalidInput:
            return "Invalid Input (N = 0 or N is odd)";
        case SpectrumError::AllocationFailed:
            return "FFT Allocation Failed";
        default:
            return "Unknown Spectrum Error";
        }
    }
    inline static QString toQstring(WaveGenerateError error) {
        switch (error) {
        case AnalysisUtils::WaveGenerateError::SpectrumCalculationFailed:
            return "Spectrum Calculation Failed";
        case AnalysisUtils::WaveGenerateError::NoSignificantFound:
            return "No Significant Peak Found";
        default:
            return "Unknown Wave Generation Error";
        }
    }

    // 지정된 차수의 고조파 성분 결과를 반환 (없으면 nullptr)
    static const HarmonicAnalysisResult* getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order);

    // 두 고조파 성분으로부터 유효 전력을 계산
    static double calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp);

    // 가장 지배적인 고조파 성분을 찾고 반환 (없으면 nullptr)
    static const HarmonicAnalysisResult* getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics);

    static std::expected<std::vector<std::complex<double>>, SpectrumError> calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow);

    static std::expected<std::vector<double>, WaveGenerateError> generateFundamentalWave(const std::vector<DataPoint>& samples);

    static std::vector<HarmonicAnalysisResult> findSignificantHarmonics(const std::vector<std::complex<double>>& spectrum);

    static PhaseData calculateActivePower(const std::vector<DataPoint>& samples);

    static PhaseData calculateTotalRms(const std::vector<DataPoint>&samples, DataType type);

    static MeasuredData buildMeasuredData(
        const PhaseData& totalVoltageRms,
        const PhaseData& totalCurrentRms,
        const PhaseData& activePower,
        const std::vector<HarmonicAnalysisResult>& voltageHarmonics,
        const std::vector<HarmonicAnalysisResult>& currentHarmonics
    );

    static OneSecondSummaryData buildOneSecondSummary(const std::vector<MeasuredData>& cycleBuffer);

private:
    static std::map<int, kiss_fftr_cfg> m_fftConfigCache;
};

#endif // ANALYSIS_UTILS_H
