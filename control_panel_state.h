#ifndef CONTROL_PANEL_STATE_H
#define CONTROL_PANEL_STATE_H

#include "config.h"
#include "shared_data_types.h"

struct ControlPanelState {
    // Source
    struct Source {
        double amplitude = config::Source::Amplitude::Default;
        double currentAmplitude = config::Source::Current::DefaultAmplitude;
        double frequency = config::Source::Frequency::Default;
        double currentPhaseDegrees = config::Source::Current::DefaultPhaseOffset;
    } source;

    // three phase
    struct ThreePhase {
        double voltageBAmplitude = config::Source::ThreePhase::DefaultAmplitudeB;
        double voltageBPhase = config::Source::ThreePhase::DefaultPhaseB_deg;
        double voltageCAmplitude = config::Source::ThreePhase::DefaultAmplitudeC;
        double voltageCPhase = config::Source::ThreePhase::DefaultPhaseC_deg;

        double currentBAmplitude = config::Source::ThreePhase::DefaultCurrentAmplitudeB;
        double currentBPhase = config::Source::ThreePhase::DefaultCurrentPhaseB_deg;
        double currentCAmplitude = config::Source::ThreePhase::DefaultCurrentAmplitudeC;
        double currentCPhase =  config::Source::ThreePhase::DefaultCurrentPhaseC_deg;
    } threePhase;

    // Simulation
    struct Simulation {
        double timeScale = config::TimeScale::Default;
        double samplingCycles = config::Sampling::DefaultSamplingCycles;
        int samplesPerCycle = config::Sampling::DefaultSamplesPerCycle;
        UpdateMode updateMode = UpdateMode::PerCycle;
        int maxDataSize = config::Simulation::DataSize::DefaultDataSize;
    } simulation;

    // harmonics
    struct Harmonics {
        HarmonicList voltageList;
        HarmonicList currentList;
    } harmonics;

    // UI
    struct View {
        bool isRunning = false;
        double graphWidth = config::Simulation::GraphWidth::Default;
    } view;
};

#endif // CONTROL_PANEL_STATE_H
