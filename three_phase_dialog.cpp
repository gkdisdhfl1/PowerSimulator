#include "three_phase_dialog.h"
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>

ThreePhaseDialog::ThreePhaseDialog(QWidget *parent)
    : QDialog{parent}
{
    setupUi();

    for(int i{0}; i < ParamCount; ++i) {
        connect(m_spinboxes[i], &QDoubleSpinBox::valueChanged, this, [this, i](double value) {
            emit valueChanged(i, value);
        });
    }
}

void ThreePhaseDialog::setupUi()
{
    setWindowTitle("3상 상세 설정");
    auto* voltageGroup = new QGroupBox("전압");
    auto* voltageLayout = new QFormLayout();
    const std::vector<QString> v_labels = {
        "B상 크기 (V)",
        "B상 위상 (°)",
        "C상 크기 (V)",
        "C상 위상 (°)",
    };

    for(int i{0}; i < 4; ++i) {
        m_spinboxes[i] = new QDoubleSpinBox();
        m_spinboxes[i]->setKeyboardTracking(false); // 키보드 입력중에 valuedChanged 시그널 발생 안함
        voltageLayout->addRow(v_labels[i], m_spinboxes[i]);
    }
    voltageGroup->setLayout(voltageLayout);

    auto* currentGroup = new QGroupBox("전류");
    auto* currentLayout = new QFormLayout();
    const std::vector<QString> i_labels = {
        "B상 크기 (A)",
        "B상 위상 (°)",
        "C상 크기 (A)",
        "C상 위상 (°)",
    };

    for(int i{0}; i < 4; ++i) {
        m_spinboxes[i + 4] = new QDoubleSpinBox();
        currentLayout->addRow(i_labels[i], m_spinboxes[i + 4]);
    }
    currentGroup->setLayout(currentLayout);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(voltageGroup);
    mainLayout->addWidget(currentGroup);
}

void ThreePhaseDialog::setInitialValues(const SimulationEngine::Parameters& params)
{
    QSignalBlocker blocker(this);
    m_spinboxes[VoltageBAmplitude]->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_spinboxes[VoltageBAmplitude]->setValue(params.voltage_B_amplitude);
    m_spinboxes[VoltageCAmplitude]->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_spinboxes[VoltageCAmplitude]->setValue(params.voltage_C_amplitude);
    m_spinboxes[VoltageBPhase]->setRange(-359, 359);
    m_spinboxes[VoltageBPhase]->setValue(params.voltage_B_phase_deg);
    m_spinboxes[VoltageCPhase]->setRange(-359, 359);
    m_spinboxes[VoltageCPhase]->setValue(params.voltage_C_phase_deg);

    m_spinboxes[CurrentBAmplitude]->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_spinboxes[CurrentBAmplitude]->setValue(params.current_B_amplitude);
    m_spinboxes[CurrentCAmplitude]->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    m_spinboxes[CurrentCAmplitude]->setValue(params.current_C_amplitude);
    m_spinboxes[CurrentBPhase]->setRange(-359, 359);
    m_spinboxes[CurrentBPhase]->setValue(params.current_B_phase_deg);
    m_spinboxes[CurrentCPhase]->setRange(-359, 359);
    m_spinboxes[CurrentCPhase]->setValue(params.current_C_phase_deg);
}
