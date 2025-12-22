#ifndef CONTROL_PANEL_STATE_H
#define CONTROL_PANEL_STATE_H

#include "shared_data_types.h"

struct ControlPanelState {
    // Source 파라미터
    double amplitude;
    double currentAmplitude;
    double frequency;
    double currentPhaseDegrees;

    // Simulation 파라미터
    double timeScale;
    double samplingCycles;
    int samplesPerCycle;

    // 고조파 성분
    HarmonicList voltageHarmonics;
    HarmonicList currentHarmonics;

    // UI 상태
    bool isRunning;
    UpdateMode updateMode;
};

#endif // CONTROL_PANEL_STATE_H
