#ifndef CONTROL_PANEL_STATE_H
#define CONTROL_PANEL_STATE_H

#include "simulation_engine.h"

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

    // UI 상태
    bool isRunning;
    SimulationEngine::UpdateMode updateMode;
};

#endif // CONTROL_PANEL_STATE_H
