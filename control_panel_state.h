#ifndef CONTROL_PANEL_STATE_H
#define CONTROL_PANEL_STATE_H

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
    int updateMode; // 0: perSample, 1: perHalfCycle, 2: perCycle
};

#endif // CONTROL_PANEL_STATE_H
