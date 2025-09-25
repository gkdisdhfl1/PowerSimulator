#include "AnalysisUtils.h"
#include "config.h"
#include <complex>

namespace {
HarmonicAnalysisResult createHarmonicResult(const std::vector<std::complex<double>>& spectrum, int order)
{
    if(order <= 0 || order >= spectrum.size()) return {};

    const auto& phasorRms = spectrum[order];
    const double rms = std::abs(phasorRms);

    return {
        .order = order,
        .rms = rms,
        .phase = std::arg(phasorRms),
        .phasorX = phasorRms.real(),
        .phasorY = phasorRms.imag()
    };
}
}

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
            const double two_pi_over_N_minus_1 = config::Math::TwoPi / (N - 1);
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
        const double two_pi_over_N = config::Math::TwoPi / N;

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
        const double two_pi_over_N = config::Math::TwoPi / N;

        for(size_t n = 0; n < N; ++n) {
            double value = amplitude * std::cos(fundamental_k * two_pi_over_N * n + phase);
            clean_wave.push_back(value);
        }

        return clean_wave;
    }

    std::vector<HarmonicAnalysisResult> findSignificantHarmonics(const std::vector<std::complex<double>>& spectrum) {
        {
            std::vector<HarmonicAnalysisResult> results;
            if(spectrum.size() < 2) return results;

            // 기본파는 항상 결과에 추가
            results.push_back(createHarmonicResult(spectrum, 1));

            // 나머지 중에서 가장 큰 고조파를 찾음
            int harmonicOrder = -1;
            double maxHarmonicMagSq = -1.0;

            for(size_t k = 2; k < spectrum.size(); ++k) {
                double magSq = std::norm(spectrum[k]); // 크기의 제곱으로 비교
                if(magSq > maxHarmonicMagSq) {
                    // 기존의 Fundamental을 harmonic으로 내림
                    maxHarmonicMagSq= magSq;
                    harmonicOrder = k;
                }
            }

            // 노이즈와 구분하기 위한 최소 임계값
            // 기본파 RMS의 1%보다 커야 함
            const double fundamentalRmsSq = std::norm(spectrum.size() > 1 ? spectrum[1] : 0);
            const double noiseThresholdSq = fundamentalRmsSq * 0.0001;

            // 유의미한 고조파 결과 추가
            if(harmonicOrder != -1 && maxHarmonicMagSq > noiseThresholdSq) {
                results.push_back(createHarmonicResult(spectrum, harmonicOrder));
            }

            return results;
        }
    }


}

