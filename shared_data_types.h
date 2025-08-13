#ifndef SHARED_DATA_TYPES_H
#define SHARED_DATA_TYPES_H

#include <QString>

// 프리셋 데이터의 키와 UI에 표시될 이름을 정의하는 상수 네임스페이스
namespace PresetKeys {
    const QString Amplitude         = "진폭 (Voltage)";
    const QString CurrentAmplitude  = "진폭 (Current)";
    const QString Frequency         = "주파수";
    const QString CurrentPhase      = "위상차";
    const QString TimeScale         = "시간 배율";
    const QString SamplingCycles    = "초당 cycle";
    const QString SamplesPerCycle   = "cycle당 sample";
    const QString MaxDataSize       = "데이터 최대 개수";
    const QString GraphWidth        = "그래프 시간 폭";
    const QString UpdateMode        = "갱신 모드";
}

#endif // SHARED_DATA_TYPES_H
