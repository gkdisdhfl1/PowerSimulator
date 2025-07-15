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

    // 슬라이더가 움직이는 중이면
    connect(ui->valueSlider, &QSlider::valueChanged, this, &ValueControlWidget::onSliderMoved);

    // 스핀박스 값을 바꾸면 onSpinBoxValueChanged 슬롯 호출
    connect(ui->valueSpinBox, &QDoubleSpinBox::editingFinished, this, [this]() {
        onSpinBoxValueChanged(ui->valueSpinBox->value());
    });
}

ValueControlWidget::~ValueControlWidget()
{
    delete ui;
}

// --- public 구현 ---
void ValueControlWidget::setRange(double min, double max)
{
    ui->valueSpinBox->setRange(min, max);
    m_firstSlideMax = max;
    m_firstSlideMin = min;
    updateUiAppearance(); // 초기 UI 설정
}

void ValueControlWidget::setValue(double value)
{
    ui->valueSpinBox->setValue(value);
    syncSliderToValue(); // 값 설정 후 슬라이더 위치 동기화
}

double ValueControlWidget::value() const
{
    return ui->valueSpinBox->value();
}

void ValueControlWidget::setSteps(double singleStep, double fineStep)
{
    m_singleStep = singleStep;
    m_fineStep = fineStep;

    updateUiAppearance();
}

// event handler 구현
void ValueControlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_isFineTuningMode = !m_isFineTuningMode; // 상태 반전

    ui->valueSlider->blockSignals(true);
    updateUiAppearance(); // UI 모양 업데이트
    syncSliderToValue(); // 새 모드에 맞게 슬라이더 위치 재설정
    ui->valueSlider->blockSignals(false);

    QWidget::mouseDoubleClickEvent(event);
}

// --- private 구현 ---
void ValueControlWidget::onSpinBoxValueChanged(double value)
{
    // 사용자가 직접 입력한 값으로 슬라이더 위치 동기화 및 외부 알림
    syncSliderToValue();
    emit valueChanged(value);
}

void ValueControlWidget::onSliderMoved(int position)
{
    double newValue = calculateNewValue(position);

    ui->valueSpinBox->blockSignals(true);
    ui->valueSpinBox->setValue(newValue);
    ui->valueSpinBox->blockSignals(false);

    emit valueChanged(ui->valueSpinBox->value());
}

void ValueControlWidget::updateUiAppearance()
{
    if(m_isFineTuningMode) {
        qDebug() << "UI Mode: Fine-tuning";
        ui->valueSpinBox->setSingleStep(m_fineStep);
        ui->valueSlider->setRange(0, 99);
        ui->valueSpinBox->setStyleSheet("background-color: #e0f0ff;");
    } else {
        qDebug() << "UI Mode: Normal";
        ui->valueSpinBox->setSingleStep(m_singleStep);
        ui->valueSlider->setRange(m_firstSlideMin, m_firstSlideMax);
        ui->valueSpinBox->setStyleSheet("");
    }

    ui->valueSlider->setSingleStep(1);
    ui->valueSlider->setPageStep(10);
}

void ValueControlWidget::syncSliderToValue()
{
    double currentValue = ui->valueSpinBox->value();
    int newSliderValue;

    if(m_isFineTuningMode) {
        // 소수점 두 자리를 정수로 변환
        double fracPart = std::abs(currentValue - std::trunc(currentValue));
        newSliderValue = static_cast<int>(std::round(fracPart * 100.0));
    } else {
        // 정수 부분만 사용
        newSliderValue = static_cast<int>(std::round(currentValue));
    }

    ui->valueSlider->blockSignals(true);
    ui->valueSlider->setValue(newSliderValue);
    ui->valueSlider->blockSignals(false);
}

double ValueControlWidget::calculateNewValue(int sliderPosition) const
{
    double currentValue = ui->valueSpinBox->value();
    double newValue;

    if(m_isFineTuningMode) {
        // 집중 모드
        double intPart = std::trunc(currentValue);
        if(currentValue >= 0)
            newValue = intPart + (static_cast<double>(sliderPosition) * 0.01);
        else
            newValue = intPart - (static_cast<double>(sliderPosition) * 0.01);
    } else {
        // 일반 모드
        double fracPart = std::abs(currentValue - std::trunc(currentValue));
        fracPart = std::round(fracPart * 100.0) / 100.0;

        if(sliderPosition >= 0)
            newValue = static_cast<double>(sliderPosition) + fracPart;
        else
            newValue = static_cast<double>(sliderPosition) - fracPart;
    }

    // 최종 값의 오차 제거 및 범위 제한
    newValue = std::round(newValue * 100.0) / 100.0;
    return std::clamp(newValue, m_firstSlideMin, m_firstSlideMax);
}
