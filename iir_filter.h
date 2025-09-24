#ifndef IIR_FILTER_H
#define IIR_FILTER_H

#include <numbers>
#include <cmath>
#include <vector>

namespace Iir {
    // 2차 IIR 필터(Biquad) 계수
struct BiquadCoeffs {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;

};

// 필터 상태 (이전 입출력 값 저장용)
struct BiquadState {
    double x1 = 0.0, x2 = 0.0; // previous input
    double y1 = 0.0, y2 = 0.0; // previous output
};

// LPF 계산 함수
// sampleRate: 샘플링 주파수, cutoffFreq: 차단 주파수
// q: Q-factor (대역폭 제어)
inline BiquadCoeffs calcLowpassCoeffs(double sampleRate, double cutoffFreq, double q = 0.7071) {
    BiquadCoeffs coeffs;
    if(sampleRate <= 0 || cutoffFreq <= 0 || cutoffFreq >= sampleRate / 2) {
        // 잘못된 매개변수일 경우 필터링 안함
        return coeffs;
    }

    const double w0 = 2.0 * std::numbers::pi * cutoffFreq / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cos_w0 = std::cos(w0);

    const double a0 = 1.0 + alpha;

    coeffs.b0 = (1.0 - cos_w0) / 2.0 / a0;
    coeffs.b1 = (1.0 - cos_w0) / a0;
    coeffs.b2 = (1.0 - cos_w0) / 2.0 / a0;
    coeffs.a1 = -2.0 * cos_w0 / a0;
    coeffs.a2 = (1.0 - alpha) / a0;

    return coeffs;
}

// 단일 샘플에 필터 적용 (direct form 2 Transposed)
inline double processSample(double input, const BiquadCoeffs& coeffs, BiquadState& state)
{
    double output = coeffs.b0 * input + state.x1;
    state.x1 = coeffs.b1 * input - coeffs.a1 * output + state.x2;
    state.x2 = coeffs.b2 * input - coeffs.a2 * output;
    return output;
}

// 벡터 데이터 전체에 필터 적용
inline std::vector<double> processVector(const std::vector<double>& input, const BiquadCoeffs& coeffs) {
    std::vector<double> output;
    output.reserve(input.size());
    BiquadState state;

    for(const auto& sample : input) {
        output.push_back(processSample(sample, coeffs, state));
    }
    return output;
}

}

#endif // IIR_FILTER_H
