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
    connect(m_closeButton, &QPushButton::clicked, this, &PidTuningDialog::accept);

    // 각 ValueControlWidget 값이 변경될 때 슬롯 호출
    connect(m_fllKpControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onFllValuesChanged);
    connect(m_fllKiControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onFllValuesChanged);
    connect(m_fllKdControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onFllValuesChanged);

    connect(m_zcKpControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onZcValuesChanged);
    connect(m_zcKiControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onZcValuesChanged);
    connect(m_zcKdControl, &ValueControlWidget::valueChanged, this, &PidTuningDialog::onZcValuesChanged);
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
    m_closeButton = new QPushButton("닫기");

    // 레이아웃 생성
    auto mainLayout = new QVBoxLayout(this);
    auto pidLayout = new QHBoxLayout();
    pidLayout->addWidget(m_fllGroup);
    pidLayout->addWidget(m_zcGroup);

    mainLayout->addLayout(pidLayout);
    mainLayout->addWidget(m_closeButton, 0, Qt::AlignRight);
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

void PidTuningDialog::onFllValuesChanged()
{
    FrequencyTracker::PidCoefficients coeffs;
    coeffs.Kp = m_fllKpControl->value();
    coeffs.Ki = m_fllKpControl->value();
    coeffs.Kd = m_fllKpControl->value();
    emit fllCoefficientsChanged(coeffs);
}

void PidTuningDialog::onZcValuesChanged()
{
    FrequencyTracker::PidCoefficients coeffs;
    coeffs.Kp = m_zcKpControl->value();
    coeffs.Ki = m_zcKpControl->value();
    coeffs.Kd = m_zcKpControl->value();
    emit zcCoefficientsChanged(coeffs);
}

void PidTuningDialog::applyChanges()
{

}
