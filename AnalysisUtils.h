#ifndef ANALYSISUTILS_H
#define ANALYSISUTILS_H

#include "data_point.h"
#include "measured_data.h"
#include "kiss_fftr.h"
#include <cmath>
#include <complex>
#include <map>

class AnalysisUtils {
public:
    enum class DataType { Voltage, Current };

    // 지정된 차수의 고조파 성분 결과를 반환 (없으면 nullptr)
    static const HarmonicAnalysisResult* getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order);

    // 두 고조파 성분으로부터 유효 전력을 계산
    static double calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp);

    // 가장 지배적인 고조파 성분을 찾고 반환 (없으면 nullptr)
    static const HarmonicAnalysisResult* getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics);

    static std::vector<std::complex<double>> calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow);

    static std::vector<double> generateFundamentalWave(const std::vector<DataPoint>& samples);

    static std::vector<HarmonicAnalysisResult> findSignificantHarmonics(const std::vector<std::complex<double>>& spectrum);

    static double calculateActivePower(const std::vector<DataPoint>& samples);

    static double calculateTotalRms(const std::vector<DataPoint>&samples, DataType type);
private:
    static std::map<int, kiss_fftr_cfg> m_fftConfigCache;
};

#endif // ANALYSISUTILS_H
