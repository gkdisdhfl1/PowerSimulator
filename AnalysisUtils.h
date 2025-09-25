#ifndef ANALYSISUTILS_H
#define ANALYSISUTILS_H

#include "data_point.h"
#include "measured_data.h"
#include <cmath>
#include <complex>


namespace AnalysisUtils {
    // 지정된 차수의 고조파 성분 결과를 반환 (없으면 nullptr)
    const HarmonicAnalysisResult* getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order);

    // 두 고조파 성분으로부터 유효 전력을 계산
    double calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp);

    // 가장 지배적인 고조파 성분을 찾고 반환 (없으면 nullptr)
    const HarmonicAnalysisResult* getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics);

    std::vector<std::complex<double>> calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow);

    std::vector<double> generateFundamentalWave(const std::vector<DataPoint>& samples);

    std::vector<HarmonicAnalysisResult> findSignificationHarmonics(const std::vector<std::complex<double>>& spectrum);
}
#endif // ANALYSISUTILS_H
