#include "value_control_widget.h"
#include "ui_value_control_widget.h"
#include "config.h"
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

    // 슬라이더가 움직이는 중이면
    connect(ui->valueSlider, &QSlider::sliderMoved, this, &ValueControlWidget::onSliderMoved);
    // 슬라이더에서 마우스를 떼면
    connect(ui->valueSlider, &QSlider::sliderReleased, this, &ValueControlWidget::onSliderReleased);

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
    ui->valueSlider->setRange(static_cast<int>(min), static_cast<int>(max));
    m_firstSlideMax = max;
    m_firstSlideMin = min;
}

void ValueControlWidget::setValue(double value)
{
    // spinBox와 slider의 값을 동시에 설정
    ui->valueSlider->setValue(qRound(value));
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
    // 사용자가 직접 입력한 값도 정밀도에 맞춰 정리
    double sanitizedValue = std::round(value * 100.0)/ 100.0;

    // 스핀박스의 표시 값도 정리된 값으로 업데이트
    if(!qFuzzyCompare(value, sanitizedValue)) {
        ui->valueSpinBox->blockSignals(true);
        ui->valueSpinBox->setValue(sanitizedValue);
        ui->valueSpinBox->blockSignals(false);
    }

    const int intValue = qRound(value);

    ui->valueSlider->blockSignals(true);
    ui->valueSlider->setValue(intValue);
    ui->valueSlider->blockSignals(false);

    emit valueChanged(value);
}

void ValueControlWidget::updateUiForTuningMode()
{
    double currentValue = ui->valueSpinBox->value();

    if(m_isFineTuningMode) {
        qDebug() << "Fine-tuning mode: ON";
        qDebug() << "currentValue: " << currentValue;


        // 집중 모드일 때 step을 fineStep으로 변경
        ui->valueSpinBox->setSingleStep(m_fineStep);

        qDebug() << "previous sliderValue: " << ui->valueSlider->value();

        int intPart = static_cast<int>(std::trunc(currentValue));
        double fracPart = std::round((currentValue - static_cast<double>(intPart)) * 100.0);
        int sliderValueForFrac = static_cast<int>(fracPart);
        qDebug() << "intPart: " << intPart;
        qDebug() << "fractionalPart: " << fracPart;

        ui->valueSlider->setRange(0,99); // 소수점 범위로.
        ui->valueSlider->setValue(abs(sliderValueForFrac));
        qDebug() << "abs(static_cast<int>(fracPart)): " << abs(static_cast<int>(fracPart));
        qDebug() << "current sliderValue: " << ui->valueSlider->value();
        ui->valueSlider->setSingleStep(1); //
        ui->valueSlider->setPageStep(10); //

        // 배경색 변경
        ui->valueSpinBox->setStyleSheet("background-color: #e0f0ff;");
    } else {
        // 일반모드 일 때 step을 singleStep으로 변경
        qDebug() << "Fine-tuning mode: OFF";
        ui->valueSpinBox->setSingleStep(m_singleStep);
        ui->valueSlider->setRange(m_firstSlideMin, m_firstSlideMax); // 범위 원래대로

        qDebug() << "abs(static_cast<int>(currentValue)): " << abs(static_cast<int>(currentValue));
        ui->valueSlider->setValue(static_cast<int>(currentValue));
        ui->valueSlider->setSingleStep(1); //
        ui->valueSlider->setPageStep(10); //

        // 배경색 변경
        ui->valueSpinBox->setStyleSheet("");
    }
}

void ValueControlWidget::onSliderMoved(int position)
{
    qDebug() << "Position: " << position;
    double currentValue = ui->valueSpinBox->value();
    qDebug() << "CurrentValue: " << currentValue;
    double newValue;

    if(m_isFineTuningMode) { // 집중 모드일 때
        double intPart = std::trunc(currentValue);
        qDebug() << "intPart: " << intPart;

        if (intPart >= 0) {
            newValue = intPart + static_cast<double>(position) * 0.01;
            qDebug() << "newValue = " << intPart << " + " << static_cast<double>(position) << "*" << 0.01;
        } else {
            newValue = intPart - static_cast<double>(position) * 0.01;
            qDebug() << "newValue = " << intPart << " - " << static_cast<double>(position) << "*" << 0.01;
        }
    } else { // 일반 모드일 때
        // 현재 spinBox의 소수 부분을 유지하면서 정수 부분만 슬라이더 값으로 변경
        double fracPart = currentValue - std::trunc(currentValue);
        // 오차 제거
        fracPart = std::round(fracPart * 100.0) / 100.0;
        qDebug() << "FracPart: " << fracPart;

        if(position >= 0) {
            if(fracPart < 0) {
                newValue = static_cast<double>(position) - fracPart;
                qDebug() << "newValue = " << static_cast<double>(position) << " - " << fracPart;
            } else {
                newValue = static_cast<double>(position) + fracPart;
                qDebug() << "newValue = " << static_cast<double>(position) << " + " << fracPart;
            }
        } else {
            if(fracPart < 0) {
                newValue = static_cast<double>(position) + fracPart;
                qDebug() << "newValue = " << static_cast<double>(position) << " + " << fracPart;
            } else {
                newValue = static_cast<double>(position) - fracPart;
                qDebug() << "newValue = " << static_cast<double>(position) << " - " << fracPart;
            }
        }

    }
    newValue = std::round(newValue * 100.0) / 100.0;
    ui->valueSpinBox->blockSignals(true);
    ui->valueSpinBox->setValue(newValue);
    ui->valueSpinBox->blockSignals(false);
}

void ValueControlWidget::onSliderReleased()
{

    qDebug() << "onSliderReleased() slider value: " << ui->valueSlider->value();
    qDebug() << "onSliderReleased() spinbox value: " << ui->valueSpinBox->value();
    emit valueChanged(ui->valueSpinBox->value());
}
