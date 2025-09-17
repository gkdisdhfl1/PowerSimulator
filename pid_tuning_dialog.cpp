#include "pid_tuning_dialog.h"
#include "value_control_widget.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

PidTuningDialog::PidTuningDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();

    // 시그널 슬롯 연결
    connect(m_applyButton, &QPushButton::clicked, this, &PidTuningDialog::accept);
    connect(m_closeButton, &QPushButton::clicked, this, &PidTuningDialog::reject);
}

void PidTuningDialog::setupUi()
{
    setWindowTitle("PID 계수 튜닝");

    // FLL 그룹 위젯 생성
    m_fllGroup = new QGroupBox("FLL (주파수) PID");
    m_fllKpControl = new ValueControlWidget();
    m_fllKiControl = new ValueControlWidget();
    m_fllKdControl = new ValueControlWidget();
    m_fllKpControl->setRange(0, 5); m_fllKpControl->setSteps(0.1, 0.01);
    m_fllKpControl->setDecimals(4);
    m_fllKiControl->setRange(0, 0.1); m_fllKiControl->setSteps(0.001, 0.0001);
    m_fllKiControl->setDecimals(6);
    m_fllKdControl->setRange(0, 5); m_fllKdControl->setSteps(0.1, 0.01);
    m_fllKdControl->setDecimals(4);

    auto fllLayout = new QFormLayout(m_fllGroup);
    fllLayout->addRow("P (Kp)", m_fllKpControl);
    fllLayout->addRow("I (Ki)", m_fllKiControl);
    fllLayout->addRow("D (Kd)", m_fllKdControl);

    // ZC 그룹 위젯 생성
    m_zcGroup = new QGroupBox("ZC (위상) PID");
    m_zcKpControl = new ValueControlWidget();
    m_zcKiControl = new ValueControlWidget();
    m_zcKdControl = new ValueControlWidget();
    m_zcKpControl->setRange(0, 1); m_zcKpControl->setSteps(0.01, 0.001);
    m_zcKpControl->setDecimals(4);
    m_zcKiControl->setRange(0, 0.001); m_zcKiControl->setSteps(0.00001, 0.000001);
    m_zcKiControl->setDecimals(7);
    m_zcKdControl->setRange(0, 1); m_zcKdControl->setSteps(0.01, 0.001);
    m_zcKdControl->setDecimals(4);

    auto zcLayout = new QFormLayout(m_zcGroup);
    zcLayout->addRow("P (Kp)", m_zcKpControl);
    zcLayout->addRow("I (Ki)", m_zcKiControl);
    zcLayout->addRow("D (Kd)", m_zcKdControl);

    // 버튼 생성
    m_applyButton = new QPushButton("적용");
    m_closeButton = new QPushButton("닫기");

    // 레이아웃 생성
    auto mainLayout = new QVBoxLayout(this);
    auto pidLayout = new QHBoxLayout();
    pidLayout->addWidget(m_fllGroup);
    pidLayout->addWidget(m_zcGroup);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(pidLayout);
    mainLayout->addLayout(buttonLayout, Qt::AlignRight);
}

void PidTuningDialog::setInitialValues(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs)
{
    // FLL 값 설정
    m_fllKpControl->setValue(fllCoeffs.Kp);
    m_fllKiControl->setValue(fllCoeffs.Ki);
    m_fllKdControl->setValue(fllCoeffs.Kd);

    // ZC 값 설정
    m_zcKpControl->setValue(zcCoeffs.Kp);
    m_zcKiControl->setValue(zcCoeffs.Ki);
    m_zcKdControl->setValue(zcCoeffs.Kd);
}

void PidTuningDialog::accept()
{
    FrequencyTracker::PidCoefficients pllCoeffs;
    pllCoeffs.Kp = m_fllKpControl->value();
    pllCoeffs.Ki = m_fllKiControl->value();
    pllCoeffs.Kd = m_fllKdControl->value();

    FrequencyTracker::PidCoefficients zcCoeffs;
    zcCoeffs.Kp = m_zcKpControl->value();
    zcCoeffs.Ki = m_zcKiControl->value();
    zcCoeffs.Kd = m_zcKdControl->value();

    emit settingsApplied(pllCoeffs, zcCoeffs);

    QDialog::accept();
}
