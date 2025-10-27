#ifndef SHARED_DATA_TYPES_H
#define SHARED_DATA_TYPES_H

#include <QString>

// 고조파 성분을 정의하는 구조체
// magnitude가 0이면 비활성화된 것으로 간주
struct HarmonicComponent {
    int order;
    double magnitude;
    double phase;

    // c++20 이상에서 defulat 비교 연산자를 사용하면 operator==와 <=>를 자동으로 생성해줌
    auto operator<=>(const HarmonicComponent&) const  = default;
};

// 시뮬레이션 화면 갱신 모드
enum class UpdateMode {
    PerSample,      // 매 샘플마다 갱신
    PerHalfCycle,   // 반 주기마다 갱신
    PerCycle        // 한 주기마다 갱신
};

// 3상 데이터를 담는 구조체
struct PhaseData {
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
};

#endif // SHARED_DATA_TYPES_H
