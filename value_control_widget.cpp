#include "value_control_widget.h"
#include <QStyle>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>

namespace {
constexpr int FineTuningSliderSteps = 100;
constexpr int FineTuningPageStep = 10;
constexpr double SliderStepValue = 1.0 / FineTuningSliderSteps;
}

ValueControlWidget::ValueControlWidget(QWidget *parent)
    : QWidget(parent)

    , m_currentMode(Mode::Normal)
{
    setupUi();

    // 초기 스텝 크기 설정
    m_spinBox->setSingleStep(m_singleStep);
    m_spinBox->setKeyboardTracking(false); // 키보드 입력 중에 valuedChanged 시그널 발생 안함

    // 슬라이더가 움직이는 중이면
    connect(m_slider, &QSlider::valueChanged, this, &ValueControlWidget::onSliderMoved);

    // 스핀박스 값이 확정되면 슬롯 호출
    // setKeyboardTracking(false) 덕분에 키보드 입력 중에 시그널이 발생하지 않음
    connect(m_spinBox, &QDoubleSpinBox::valueChanged, this, &ValueControlWidget::onSpinBoxValueChanged);

}
ValueControlWidget::~ValueControlWidget()
{

}

// --- public 구현 ---
void ValueControlWidget::setRange(double min, double max)
{
    m_spinBox->setRange(min, max);
    m_fineTuningRangeMax = max;
    m_fineTuningRangeMin = min;
    updateUiAppearance(); // 초기 UI 설정
}

void ValueControlWidget::setValue(double value)
{
    // 값의 범위를 위젯의 min/max 값으로 제한
    const double clampedValue = std::clamp(value, m_spinBox->minimum(), m_spinBox->maximum());
    m_spinBox->setValue(clampedValue);
    syncSliderToValue(); // 값 설정 후 슬라이더 위치 동기화
}

double ValueControlWidget::value() const
{
    return m_spinBox->value();
}

void ValueControlWidget::setSteps(double singleStep, double fineStep)
{
    m_singleStep = singleStep;
    m_fineStep = fineStep;

    updateUiAppearance();
}

void ValueControlWidget::setSuffix(const QString &suffix)
{
    m_suffixLabel->setText(suffix);
}

void ValueControlWidget::setDataType(DataType type)
{
    m_dataType = type;
    if(m_dataType == DataType::Integer) {
        m_spinBox->setDecimals(0);
        // 정수 모드에서는 항상 Normal 모드로 고정
        if(m_currentMode == Mode::FineTuning) {
            setMode(Mode::Normal);
        }
    } else {
        // Double 모드일 때 기본 소수점 자릿수
        m_spinBox->setDecimals(2);
    }
}

void ValueControlWidget::setDecimals(int decimals)
{
    m_spinBox->setDecimals(decimals);
}
// --------------------


// ---- protected 구현 ----
void ValueControlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 정수 모드에서는 미세 조정 모드로 진입하지 않음
    if(m_dataType == DataType::Integer) return;

    const Mode newMode = (m_currentMode == Mode::Normal) ? Mode::FineTuning : Mode::Normal;
    setMode(newMode);

    QWidget::mouseDoubleClickEvent(event);
}
// ------------------------------


// --- private 구현 ---
void ValueControlWidget::setupUi()
{
    // 위젯 생성
    m_spinBox = new QDoubleSpinBox;
    m_slider = new QSlider(Qt::Horizontal);
    m_suffixLabel = new QLabel();

    // 레이아웃 설정
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(m_spinBox, 1); // spinBox가 남는 공간 차지
    topLayout->addWidget(m_suffixLabel);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_slider);
}

void ValueControlWidget::setMode(Mode newMode)
{
    m_currentMode = newMode;

    // 동적 속성 설정 코드
    m_spinBox->setProperty("fineTuning", (newMode == Mode::FineTuning));

    // 스타일을 다시 적용하여 변경사항을 반영
    style()->unpolish(m_spinBox);
    style()->polish(m_spinBox);

    m_slider->blockSignals(true);
    updateUiAppearance();
    syncSliderToValue();
    m_slider->blockSignals(false);
}

void ValueControlWidget::updateUiAppearance()
{
    switch (m_currentMode) {
    case Mode::Normal:
        m_spinBox->setSingleStep(m_singleStep);
        m_slider->setRange(m_fineTuningRangeMin, m_fineTuningRangeMax);
        break;
    case Mode::FineTuning:
        m_spinBox->setSingleStep(m_fineStep);
        m_slider->setRange(0, FineTuningSliderSteps - 1);
        break;
    }

    m_slider->setSingleStep(1);
    m_slider->setPageStep(FineTuningPageStep);
}

void ValueControlWidget::syncSliderToValue()
{
    double currentValue = m_spinBox->value();
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

    m_slider->blockSignals(true);
    m_slider->setValue(newSliderValue);
    m_slider->blockSignals(false);
}

double ValueControlWidget::calculateNewValue(int sliderPosition) const
{
    double currentValue = m_spinBox->value();
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
// --------------------


// ---- private slot -----
void ValueControlWidget::onSpinBoxValueChanged(double value)
{
    // 사용자가 직접 입력한 값으로 슬라이더 위치 동기화 및 외부 알림
    syncSliderToValue();
    if(m_dataType == DataType::Integer) {
        emit intValueChanged(static_cast<int>(std::round(value)));
    } else {
        emit valueChanged(value);
    }
}

void ValueControlWidget::onSliderMoved(int position)
{
    double newValue = calculateNewValue(position);

    m_spinBox->blockSignals(true);
    m_spinBox->setValue(newValue);
    m_spinBox->blockSignals(false);

    if(m_dataType == DataType::Integer) {
        emit intValueChanged(static_cast<int>(std::round(newValue)));
    } else {
        emit valueChanged(newValue);
    }
}
// -----------------------
