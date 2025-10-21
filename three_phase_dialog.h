#ifndef THREE_PHASE_DIALOG_H
#define THREE_PHASE_DIALOG_H

#include <QDialog>
#include "simulation_engine.h"

class QDoubleSpinBox;

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
    std::array<QDoubleSpinBox*, ParamCount> m_spinboxes;
};

#endif // THREE_PHASE_DIALOG_H
