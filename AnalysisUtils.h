#ifndef ANALYSISUTILS_H
#define ANALYSISUTILS_H

#include "measured_data.h"
#include <cmath>


namespace AnalysisUtils {
    // 지정된 차수의 고조파 성분 결과를 반환 (없으면 nullptr)
    inline const HarmonicAnalysisResult* getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order)
    {
        auto it = std::find_if(harmonics.begin(), harmonics.end(), [order](const HarmonicAnalysisResult& h) {
            return h.order == order;
        });
        return (it != harmonics.end()) ? &(*it) : nullptr;
    }

    // 두 고조파 성분으로부터 유효 전력을 계산
    inline double calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp)
    {
        if(v_comp && i_comp) {
            return v_comp->rms * i_comp->rms * std::cos(v_comp->phase - i_comp->phase);
        }
        return 0.0;
    }

    // 가장 지배적인 고조파 성분을 찾고 반환 (없으면 nullptr)
    inline const HarmonicAnalysisResult* getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics)
    {
        const HarmonicAnalysisResult* dominant = nullptr;
        double maxRms = -1.0;

        for(const auto& h : harmonics) {
            if(h.order > 1 && h.rms > maxRms) {
                maxRms = h.rms;
                dominant = &h;
            }
        }
        return dominant;
    }
}
#endif // ANALYSISUTILS_H
