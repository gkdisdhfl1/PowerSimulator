#ifndef THREE_PHASE_DIALOG_H
#define THREE_PHASE_DIALOG_H

#include <QDialog>
#include "simulation_engine.h"

class ValueControlWidget;
class FineTuningDial;
class QLabel;

class ThreePhaseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ThreePhaseDialog(QWidget *parent = nullptr);

    enum ParamType {
        VoltageBAmplitude,
        VoltageBPhase,
        VoltageCAmplitude,
        VoltageCPhase,
        CurrentBAmplitude,
        CurrentBPhase,
        CurrentCAmplitude,
        CurrentCPhase,
        ParamCount // 배열 크기를 위해 마지막 추가
    };
    void setInitialValues(const SimulationEngine::Parameters& params);

signals:
    void valueChanged(int type, double value);


private:
    void setupUi();
    std::array<ValueControlWidget*, 4> m_amplitudeControls;
    std::array<FineTuningDial*, 4> m_phaseDials;
    std::array<QLabel*, 4> m_phaseLabels;
};

#endif // THREE_PHASE_DIALOG_H
