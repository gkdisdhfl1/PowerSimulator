#include "three_phase_dialog.h"
#include "value_control_widget.h"
#include "fine_tuning_dial.h"
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>

ThreePhaseDialog::ThreePhaseDialog(QWidget *parent)
    : QDialog{parent}
{
    setupUi();

    for(int i{0}; i < 4; ++i) {
        // Amplitude (ValueControlWidget)
        connect(m_amplitudeControls[i], &ValueControlWidget::valueChanged, this, [this, i](double value) {
            // i=0: VbAmp, i=1: VcAmp, i=2: IbAmp, i=3: IcAmp
            int type = (i < 2) ? (i * 2) : ((i - 2) * 2 + 4);
            emit valueChanged(type, value);
        });

        // Phase (FineTuningDial)
        connect(m_phaseDials[i], &FineTuningDial::valueChanged, this, [this, i](int value) {
            // i=0: VbPhase, i=1: VcPhase, i=2: IbPhase, i=3: IcPhase
            int type = (i < 2) ? (i * 2 + 1) : ((i - 2) * 2 + 5);
            emit valueChanged(type, static_cast<double>(value));
            m_phaseLabels[i]->setText(QString::number(value) + " °");
        });
    }
}

void ThreePhaseDialog::setupUi()
{
    setWindowTitle("3상 상세 설정");
    auto mainLayout = new QVBoxLayout(this);

    // 위젯 생성 및 레이아웃 구성
    const QStringList names = {"B상", "C상"};
    for(int i{0}; i < 4; ++i) {
        const bool isVoltage = (i < 2);
        const QString unit = isVoltage ? "(V)" : "(A)";
        const QString& phaseName = names[i % 2];

        m_amplitudeControls[i] = new ValueControlWidget();
        m_phaseDials[i] = new FineTuningDial();
        m_phaseLabels[i] = new QLabel("0 °");

        auto phaseLayout = new QHBoxLayout();
        phaseLayout->addWidget(m_phaseDials[i]);
        phaseLayout->addWidget(m_phaseLabels[i]);

        QString groupTitle = isVoltage ? "전압 설정" : "전류 설정";
        if(i % 2 == 0) {
            auto groupBox = new QGroupBox(groupTitle);
            auto groupLayout = new QFormLayout();
            groupLayout->addRow(phaseName + " 크기 " + unit, m_amplitudeControls[i]);
            groupLayout->addRow(phaseName + " 위상 (°) " + unit, phaseLayout);
            groupBox->setLayout(groupLayout);
            mainLayout->addWidget(groupBox);
        } else { // 이미 생성된 그룹에 추가
            QGroupBox* lastGroupBox = qobject_cast<QGroupBox*>(mainLayout->itemAt(mainLayout->count() - 1)->widget());
            QFormLayout* groupLayout = qobject_cast<QFormLayout*>(lastGroupBox->layout());
            groupLayout->addRow(phaseName + " 크기 " + unit, m_amplitudeControls[i]);
            groupLayout->addRow(phaseName + " 위상 (°) " + unit, phaseLayout);
        }
    }

    setLayout(mainLayout);
}

void ThreePhaseDialog::setInitialValues(const SimulationEngine* engine)
{
    // qDebug() << "[DBG] setInitialValues this=" << this
    //          << " dialog visible=" << this->isVisible()
    //          << " windowTitle=" << windowTitle();
    // qDebug() << "[DBG] VbPhase param =" << params.voltage_B_phase_deg;
    m_amplitudeControls[0]->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_amplitudeControls[0]->setValue(engine->m_voltage_B_amplitude.value());
    m_phaseDials[0]->setRange(0, 359);
    m_phaseDials[0]->setValue(engine->m_voltage_B_phase_deg.value());
    m_phaseDials[0]->setWrapping(true);
    m_phaseDials[0]->setNotchesVisible(true);

    m_amplitudeControls[1]->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_amplitudeControls[1]->setValue(engine->m_voltage_C_amplitude.value());
    m_phaseDials[1]->setRange(0, 359);
    m_phaseDials[1]->setValue(engine->m_voltage_C_phase_deg.value());
    m_phaseDials[1]->setWrapping(true);
    m_phaseDials[1]->setNotchesVisible(true);

    m_amplitudeControls[2]->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    m_amplitudeControls[2]->setValue(engine->m_current_B_amplitude.value());
    m_phaseDials[2]->setRange(0, 359);
    m_phaseDials[2]->setValue(engine->m_current_B_phase_deg.value());
    m_phaseDials[2]->setWrapping(true);
    m_phaseDials[2]->setNotchesVisible(true);

    m_amplitudeControls[3]->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    m_amplitudeControls[3]->setValue(engine->m_current_C_amplitude.value());
    m_phaseDials[3]->setRange(0, 359);
    m_phaseDials[3]->setValue(engine->m_current_C_phase_deg.value());
    m_phaseDials[3]->setWrapping(true);
    m_phaseDials[3]->setNotchesVisible(true);

    m_phaseLabels[0]->setText(QString::number(engine->m_voltage_B_phase_deg.value()) + " °");
    m_phaseLabels[1]->setText(QString::number(engine->m_voltage_C_phase_deg.value()) + " °");
    m_phaseLabels[2]->setText(QString::number(engine->m_current_B_phase_deg.value()) + " °");
    m_phaseLabels[3]->setText(QString::number(engine->m_current_C_phase_deg.value()) + " °");
}
