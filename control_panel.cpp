#include "control_panel.h"
#include "value_control_widget.h"
#include "fine_tuning_dial.h"
#include "config.h"

#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QGroupBox>
#include <QCheckBox>

ControlPanel::ControlPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    initializeUiValues();
    createConnections();
}

ControlPanelState ControlPanel::getState() const
{
    ControlPanelState state;
    state.amplitude = m_voltageControlWidget->value();
    state.currentAmplitude = m_currentAmplitudeControlWidget->value();
    state.frequency = m_frequencyControlWidget->value();
    state.currentPhaseDegrees = m_currentPhaseDial->value();
    state.timeScale = m_timeScaleControlWidget->value();
    state.samplingCycles = m_samplingCyclesControlWidget->value();
    state.samplesPerCycle = static_cast<int>(m_samplesPerCycleControlWidget->value());

    if(m_perSampleRadioButton->isChecked()) {
        state.updateMode = SimulationEngine::UpdateMode::PerSample;
    } else if(m_perHalfCycleRadioButton->isChecked()) {
        state.updateMode = SimulationEngine::UpdateMode::PerHalfCycle;
    } else {
        state.updateMode = SimulationEngine::UpdateMode::PerCycle;
    }

    state.isRunning = (m_startStopButton->text() == "일시정지");

    return state;
}

void ControlPanel::setState(const ControlPanelState& state)
{
    // ControlPanelState 객체의 값으로 모든 자식 위젯들의 상태를 업데이트
    m_voltageControlWidget->setValue(state.amplitude);
    m_currentAmplitudeControlWidget->setValue(state.currentAmplitude);
    m_frequencyControlWidget->setValue(state.frequency);
    m_currentPhaseDial->setValue(state.currentPhaseDegrees);
    m_timeScaleControlWidget->setValue(state.timeScale);
    m_samplingCyclesControlWidget->setValue(state.samplingCycles);
    m_samplesPerCycleControlWidget->setValue(state.samplesPerCycle);

    if(state.updateMode == SimulationEngine::UpdateMode::PerSample) {
        m_perSampleRadioButton->setChecked(true);
    } else if(state.updateMode == SimulationEngine::UpdateMode::PerHalfCycle) {
        m_perHalfCycleRadioButton->setChecked(true);
    } else {
        m_perCycleRadioButton->setChecked(true);
    }

    setRunningState(state.isRunning);
}

void ControlPanel::setupUi()
{
    // 위젯 인스턴스 생성 (최종 이름 규칙 적용)
    m_startStopButton = new QPushButton("시작");
    m_settingButton = new QPushButton("설정");

    m_voltageControlWidget = new ValueControlWidget();
    m_currentAmplitudeControlWidget = new ValueControlWidget();
    m_frequencyControlWidget = new ValueControlWidget();
    m_timeScaleControlWidget = new ValueControlWidget();
    m_samplingCyclesControlWidget = new ValueControlWidget();
    m_samplesPerCycleControlWidget = new ValueControlWidget();

    m_currentPhaseDial = new FineTuningDial();
    m_currentPhaseLabel = new QLabel();

    m_perSampleRadioButton = new QRadioButton("1 Sample");
    m_perHalfCycleRadioButton = new QRadioButton("Half Cycle");
    m_perCycleRadioButton = new QRadioButton("1 Cycle");

    m_autoScrollCheckBox = new QCheckBox("자동 스크롤");

    // 레이아웃 설정
    auto mainLayout = new QVBoxLayout(this);

    // 버튼 레이아웃
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    buttonLayout->addWidget(m_startStopButton);
    buttonLayout->addWidget(m_settingButton);
    buttonLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // 파라미터 폼 레이아웃
    auto formLayout = new QFormLayout();
    formLayout->addRow("진폭", m_voltageControlWidget);
    formLayout->addRow("전류 진폭", m_currentAmplitudeControlWidget);
    formLayout->addRow("주파수", m_frequencyControlWidget);
    formLayout->addRow("시간 배율", m_timeScaleControlWidget);
    formLayout->addRow("초당 cycle", m_samplingCyclesControlWidget);
    formLayout->addRow("cycle당 sample", m_samplesPerCycleControlWidget);
    // formLayout->addRow(m_currentPhaseLabel, m_currentPhaseDial);

    // 위상차 다이얼 레이아웃
    auto phaseLayout = new QHBoxLayout();
    phaseLayout->addWidget(m_currentPhaseDial, 1);
    phaseLayout->addWidget(m_currentPhaseLabel, 0, Qt::AlignCenter);

    auto phaseGroupBox = new QGroupBox("전류 위상차");
    phaseGroupBox->setLayout(phaseLayout);

    // 화면 갱신 그룹박스 레이아웃
    auto updateModeLayout = new QHBoxLayout();
    updateModeLayout->addWidget(m_perSampleRadioButton);
    updateModeLayout->addWidget(m_perHalfCycleRadioButton);
    updateModeLayout->addWidget(m_perCycleRadioButton);
    updateModeLayout->addStretch(); // 위젯들을 왼쪽으로 밀착
    updateModeLayout->addWidget(m_autoScrollCheckBox);

    auto updateModeGroupBox = new QGroupBox("화면 갱신");
    updateModeGroupBox->setLayout(updateModeLayout);

    // 메인 레이아웃에 추가
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(phaseGroupBox);
    mainLayout->addWidget(updateModeGroupBox);
    mainLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
}

void ControlPanel::initializeUiValues()
{
    m_voltageControlWidget->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_voltageControlWidget->setValue(config::Source::Amplitude::Default);
    m_voltageControlWidget->setSuffix(" V");

    m_currentAmplitudeControlWidget->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    m_currentAmplitudeControlWidget->setValue(config::Source::Current::DefaultAmplitude);
    m_currentAmplitudeControlWidget->setSuffix(" A");

    m_frequencyControlWidget->setRange(config::Source::Frequency::Min, config::Source::Frequency::Max);
    m_frequencyControlWidget->setValue(config::Source::Frequency::Default);
    m_frequencyControlWidget->setSuffix(" Hz");

    m_timeScaleControlWidget->setRange(config::TimeScale::Min, config::TimeScale::Max);
    m_timeScaleControlWidget->setValue(config::TimeScale::Default);
    m_timeScaleControlWidget->setSuffix(" x");

    m_samplingCyclesControlWidget->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    m_samplingCyclesControlWidget->setValue(config::Sampling::DefaultSamplingCycles);
    m_samplesPerCycleControlWidget->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    m_samplesPerCycleControlWidget->setValue(config::Sampling::DefaultSamplesPerCycle);
    m_samplesPerCycleControlWidget->setDataType(ValueControlWidget::DataType::Integer);

    m_currentPhaseDial->setRange(0, 359);
    m_currentPhaseDial->setValue(config::Source::Current::DefaultPhaseOffset);
    m_currentPhaseDial->setWrapping(true);
    m_currentPhaseDial->setNotchesVisible(true);
    updateCurrentPhaseLabel(m_currentPhaseDial->value());

    QFontMetrics fm(m_currentPhaseLabel->font());
    int width = fm.horizontalAdvance("359 °"); // 가장 긴 텍스트의 너비 계산
    m_currentPhaseLabel->setMinimumWidth(width);

    m_perSampleRadioButton->setChecked(true);
    m_autoScrollCheckBox->setChecked(true);
}

void ControlPanel::createConnections()
{
    // 버튼 클릭 연결
    connect(m_startStopButton, &QPushButton::clicked, this, &ControlPanel::startStopClicked);
    connect(m_settingButton, &QPushButton::clicked, this, &ControlPanel::settingsClicked);

    // 파라미터 변경
    connect(m_voltageControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::amplitudeChanged);
    connect(m_currentAmplitudeControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::currentAmplitudeChanged);
    connect(m_frequencyControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::frequencyChanged);
    connect(m_currentPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::currentPhaseChanged);
    connect(m_timeScaleControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::timeScaleChanged);
    connect(m_samplingCyclesControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::samplingCyclesChanged);
    connect(m_samplesPerCycleControlWidget, &ValueControlWidget::intValueChanged, this, &ControlPanel::samplesPerCycleChanged);

    // 라디오 버튼 연결
    connect(m_perSampleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perHalfCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);

    // 내부 UI 업데이트 연결
    connect(m_currentPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::updateCurrentPhaseLabel);

    // 체크박스 시그널 연결
    connect(m_autoScrollCheckBox, &QCheckBox::toggled, this, &ControlPanel::autoScrollToggled);
}

// --- public slots ---
void ControlPanel::setRunningState(bool isRunning)
{
    m_startStopButton->setText(isRunning ? "일시정지" : "시작");
}

void ControlPanel::setAutoScroll(bool enabled)
{
    m_autoScrollCheckBox->setChecked(enabled);
}

// --- private slots ---
void ControlPanel::updateCurrentPhaseLabel(int value)
{
    m_currentPhaseLabel->setText(QString::number(value) + " °");
}
