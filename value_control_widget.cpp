#include "value_control_widget.h"
#include "ui_value_control_widget.h"
#include <QDebug>
#include <QStyle>

namespace {
constexpr int FineTuningSliderSteps = 100;
constexpr int FineTuningPageStep = 10;
constexpr double SliderStepValue = 1.0 / FineTuningSliderSteps;
}

ValueControlWidget::ValueControlWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ValueControlWidget)
    , m_currentMode(Mode::Normal)
{
    ui->setupUi(this);

    // 초기 스텝 크기 설정
    ui->valueSpinBox->setSingleStep(m_singleStep);
    ui->valueSpinBox->setKeyboardTracking(false); // 키보드 입력 중에 valuedChanged 시그널 발생 안함

    // 슬라이더가 움직이는 중이면
    connect(ui->valueSlider, &QSlider::valueChanged, this, &ValueControlWidget::onSliderMoved);

    // 스핀박스 값이 확정되면 슬롯 호출
    // setKeyboardTracking(false) 덕분에 키보드 입력 중에 시그널이 발생하지 않음
    connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, &ValueControlWidget::onSpinBoxValueChanged);

}

ValueControlWidget::~ValueControlWidget()
{
    delete ui;
}

// --- public 구현 ---
void ValueControlWidget::setRange(double min, double max)
{
    ui->valueSpinBox->setRange(min, max);
    m_fineTuningRangeMax = max;
    m_fineTuningRangeMin = min;
    updateUiAppearance(); // 초기 UI 설정
}

void ValueControlWidget::setValue(double value)
{
    // 값의 범위를 위젯의 min/max 값으로 제한
    const double clampedValue = std::clamp(value, ui->valueSpinBox->minimum(), ui->valueSpinBox->maximum());
    ui->valueSpinBox->setValue(clampedValue);
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

void ValueControlWidget::setSuffix(const QString &suffix)
{
    ui->suffixLabel->setText(suffix);
}

// event handler 구현
void ValueControlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    const Mode newMode = (m_currentMode == Mode::Normal) ? Mode::FineTuning : Mode::Normal;
    setMode(newMode);

    QWidget::mouseDoubleClickEvent(event);
}

// --- private 구현 ---
void ValueControlWidget::setMode(Mode newMode)
{
    m_currentMode = newMode;

    // 동적 속성 설정 코드
    ui->valueSpinBox->setProperty("fineTuning", (newMode == Mode::FineTuning));

    // 스타일을 다시 적용하여 변경사항을 반영
    style()->unpolish(ui->valueSpinBox);
    style()->polish(ui->valueSpinBox);

    ui->valueSlider->blockSignals(true);
    updateUiAppearance();
    syncSliderToValue();
    ui->valueSlider->blockSignals(false);
}

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
    switch (m_currentMode) {
    case Mode::Normal:
        qDebug() << "UI Mode: Normal";
        ui->valueSpinBox->setSingleStep(m_singleStep);
        ui->valueSlider->setRange(m_fineTuningRangeMin, m_fineTuningRangeMax);
        break;
    case Mode::FineTuning:
        qDebug() << "UI Mode: Fine-tuning";
        ui->valueSpinBox->setSingleStep(m_fineStep);
        ui->valueSlider->setRange(0, FineTuningSliderSteps - 1);
        break;
    }

    ui->valueSlider->setSingleStep(1);
    ui->valueSlider->setPageStep(FineTuningPageStep);
}

void ValueControlWidget::syncSliderToValue()
{
    double currentValue = ui->valueSpinBox->value();
    int newSliderValue;

    switch (m_currentMode) {
    case Mode::Normal:
        // 정수 부분만 사용
        newSliderValue = static_cast<int>(std::round(currentValue));
        break;
    case Mode::FineTuning: {
        // 소수점 두 자리를 정수로 변환
        double fracPart = getFractionalPart(currentValue);
        newSliderValue = static_cast<int>(std::round(fracPart * 100.0));
    }
        break;
    }

    ui->valueSlider->blockSignals(true);
    ui->valueSlider->setValue(newSliderValue);
    ui->valueSlider->blockSignals(false);
}

double ValueControlWidget::calculateNewValue(int sliderPosition) const
{
    double currentValue = ui->valueSpinBox->value();
    double newValue;

    switch (m_currentMode) {
    case Mode::Normal:
        {
            double fracPart = getFractionalPart(currentValue);
            fracPart = std::round(fracPart * 100.0) / 100.0;

            if(sliderPosition >= 0)
                newValue = static_cast<double>(sliderPosition) + fracPart;
            else
                newValue = static_cast<double>(sliderPosition) - fracPart;
        }
            break;
    case Mode::FineTuning:
        {
            double intPart = std::trunc(currentValue);
            double finePart = static_cast<double>(sliderPosition) * SliderStepValue;

            if(currentValue >= 0)
                newValue = intPart + finePart;
            else
                newValue = intPart - finePart;
        }
        break;
    }

    // 최종 값의 오차 제거 및 범위 제한
    newValue = std::round(newValue * 100.0) / 100.0;
    return std::clamp(newValue, m_fineTuningRangeMin, m_fineTuningRangeMax);
}

inline double ValueControlWidget::getFractionalPart(double value) const
{
    return std::abs(std::fmod(value, 1.0));
}
