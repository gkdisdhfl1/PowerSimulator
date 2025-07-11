#include "value_control_widget.h"
#include "ui_value_control_widget.h"
#include <QDebug>

ValueControlWidget::ValueControlWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ValueControlWidget)
{
    ui->setupUi(this);

    // 초기 스텝 크기 설정
    ui->valueSpinBox->setSingleStep(m_singleStep);
    ui->valueSlider->setSingleStep(m_singleStep * m_multiplier);
    ui->valueSlider->setPageStep(m_singleStep * m_multiplier * 10);

    // 슬라이더가 움직이면 onSliderValueChanged 슬롯 호출
    connect(ui->valueSlider, &QSlider::valueChanged, this, &ValueControlWidget::onSpinBoxValueChanged);

    // 스핀박스 값을 바꾸면 onSpinBoxValueChanged 슬롯 호출
    connect(ui->valueSpinBox, &QDoubleSpinBox::editingFinished, this, &ValueControlWidget::onSliderValueChanged);
}

ValueControlWidget::~ValueControlWidget()
{
    delete ui;
}

// --- public 구현 ---
void ValueControlWidget::setRange(double min, double max)
{
    ui->valueSpinBox->setRange(min, max);
    // 슬라이더는 정수 범위만 허용하므로, m_multiplier를 곱해서 범위 확장
    ui->valueSlider->setRange(min * m_multiplier, max * m_multiplier);
}

void ValueControlWidget::setValue(double value)
{
    // spinBox와 slider의 값을 동시에 설정
    ui->valueSlider->setValue(value * m_multiplier);
    ui->valueSpinBox->setValue(value);
}

double ValueControlWidget::value() const
{
    return ui->valueSpinBox->value();
}

void ValueControlWidget::setSteps(double singleStep, double fineStep)
{
    m_singleStep = singleStep;
    m_fineStep = fineStep;

    // 새로운 기본 스텝으로 ui 업데이트
    updateUiForTuningMode();
}

// event handler 구현
void ValueControlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_isFineTuningMode = !m_isFineTuningMode; // 상태 반전
    updateUiForTuningMode();

    QWidget::mouseDoubleClickEvent(event);
}

// --- private 구현 ---
void ValueControlWidget::onSliderValueChanged(int value)
{
    // 슬라이더 정수 값을 double로 변환
    double doubleValue = value / m_multiplier;

    // 스핀 박스의 현재 값과 다를 때만 업데이트
    if(qFuzzyCompare(ui->valueSpinBox->value(), doubleValue))
        return;

    // 스핀 박스의 값 업데이트
    ui->valueSpinBox->setValue(doubleValue);
}

void ValueControlWidget::onSpinBoxValueChanged(double value)
{
    // 스핀박스의 정수 값을 int로 변환
    int intValue = value * m_multiplier;

    // 값이 다를 때만 업데이트
    if(ui->valueSlider->value() == intValue)
        return;

    // slider 값 업데이트
    ui->valueSlider->setValue(intValue);

    emit valueChanged(value);
}

void ValueControlWidget::updateUiForTuningMode()
{
    if(m_isFineTuningMode) {
        qDebug() << "Fine-tuning mode: ON";

        // 집중 모드일 때 step을 fineStep으로 변경
        ui->valueSpinBox->setSingleStep(m_fineStep);
        ui->valueSlider->setSingleStep(m_fineStep * m_multiplier);
        ui->valueSlider->setPageStep(m_fineStep * m_multiplier * 10);

        // 배경색 변경
        ui->valueSpinBox->setStyleSheet("background-color: #e0f0ff;");
    } else {
        // 일반모드 일 때 step을 singleStep으로 변경
        qDebug() << "Fine-tuning mode: OFF";
        ui->valueSpinBox->setSingleStep(m_singleStep);
        ui->valueSlider->setSingleStep(m_singleStep * m_multiplier);
        ui->valueSlider->setPageStep(m_singleStep * m_multiplier * 10);

        // 배경색 변경
        ui->valueSpinBox->setStyleSheet("");
    }
}
