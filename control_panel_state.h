#ifndef CONTROL_PANEL_STATE_H
#define CONTROL_PANEL_STATE_H

#include "simulation_engine.h"

// 고조파 성분
struct HarmonicComponent {
    int order;
    double magnitude;
    double phase;
};

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
    HarmonicComponent voltageHarmonic;
    HarmonicComponent currentHarmonic;

    // UI 상태
    bool isRunning;
    SimulationEngine::UpdateMode updateMode;
};

#endif // CONTROL_PANEL_STATE_H
