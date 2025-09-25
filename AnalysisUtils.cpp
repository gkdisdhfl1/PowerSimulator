#include "AnalysisUtils.h"
#include <numbers>
#include <complex>

namespace AnalysisUtils {

    const HarmonicAnalysisResult* getHarmonicComponent(const std::vector<HarmonicAnalysisResult>& harmonics, int order)
    {
        auto it = std::find_if(harmonics.begin(), harmonics.end(), [order](const HarmonicAnalysisResult& h) {
            return h.order == order;
        });
        return (it != harmonics.end()) ? &(*it) : nullptr;
    }

    double calculateActivePower(const HarmonicAnalysisResult* v_comp, const HarmonicAnalysisResult* i_comp)
    {
        if(v_comp && i_comp) {
            return v_comp->rms * i_comp->rms * std::cos(v_comp->phase - i_comp->phase);
        }
        return 0.0;
    }

    const HarmonicAnalysisResult* getDominantHarmonic(const std::vector<HarmonicAnalysisResult>& harmonics)
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

    std::vector<std::complex<double>> calculateSpectrum(const std::vector<DataPoint>& samples, bool useWindow)
    {
        const size_t N = samples.size();
        if(N < 2) {
            return {};
        }

        // 임시 데이터 벡터 생성
        std::vector<double> processed_wave(N);

        if(useWindow) {
            // Hann 윈도우 적용
            const double two_pi_over_N_minus_1 = 2.0 * std::numbers::pi / (N - 1);
            for(size_t n = 0; n < N; ++n) {
                double hann_multiplier = 0.5 * (1.0 - std::cos(n * two_pi_over_N_minus_1));
                processed_wave[n] = samples[n].voltage * hann_multiplier;
            }
        } else {
            for(size_t n = 0; n < N; ++n) {
                processed_wave[n] = samples[n].voltage;
            }
        }

        // DFT 실행
        const size_t spectrumSize = N / 2 + 1;
        std::vector<std::complex<double>> spectrum(spectrumSize, {0.0, 0.0});
        const double two_pi_over_N = 2.0 * std::numbers::pi / N;

        for(size_t k = 1; k < N / 2; ++k) {
            double real_sum = 0.0;
            double imag_sum = 0.0;

            for(size_t n = 0; n < N; ++n) {
                const double angle = k * two_pi_over_N * n;
                real_sum += processed_wave[n] * cos(angle);
                imag_sum -= processed_wave[n] * sin(angle);
            }

            // DFT 정규화
            const double normFactor = std::sqrt(2.0) / N;
            spectrum[k] = {normFactor * real_sum, normFactor * imag_sum};
        }
        return spectrum;
    }

    std::vector<double> generateFundamentalWave(const std::vector<DataPoint>& samples)
    {
        const size_t N = samples.size();
        if(N < 2) {
            return {};
        }

        // 1. 스펙트럼 계산
        auto spectrum = calculateSpectrum(samples, true);
        if(spectrum.empty()) {
            return {};
        }

        // 2. 가장 강한 피크 찾기
        int fundamental_k = -1; // 기본 주파수 인덱스
        double max_magnitude_sq = -1.0;
        std::complex<double> fundamental_phasor = {0.0, 0.0};

        for(size_t k = 1; k < spectrum.size(); ++k) {
            double mag_sq = std::norm(spectrum[k]);
            if(mag_sq > max_magnitude_sq) {
                max_magnitude_sq = mag_sq;
                fundamental_k = k;
                fundamental_phasor = spectrum[k];
            }
        }

        if(fundamental_k == -1 || max_magnitude_sq < 1e-9) {
            return {}; // 유의미한 주파수를 못찾음
        }

        // 3. 기본파 재구성
        std::vector<double> clean_wave;
        clean_wave.reserve(N);

        const double amplitude = std::abs(fundamental_phasor) * sqrt(2.0);
        const double phase = std::arg(fundamental_phasor);
        const double two_pi_over_N = 2.0 * std::numbers::pi / N;

        for(size_t n = 0; n < N; ++n) {
            double value = amplitude * std::cos(fundamental_k * two_pi_over_N * n + phase);
            clean_wave.push_back(value);
        }

        return clean_wave;
    }

}

