#include "control_panel.h"
#include "value_control_widget.h"
#include "fine_tuning_dial.h"
#include "config.h"
#include "collapsible_groupbox.h"
#include "shared_data_types.h"

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
#include <QScrollArea>
#include <QTabWidget>

ControlPanel::ControlPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    setupWidgetProperties();
    createConnections();
}

ControlPanelState ControlPanel::getState() const
{
    ControlPanelState state;
    state.source.amplitude = m_voltageControlWidget->value();
    state.source.currentAmplitude = m_currentAmplitudeControlWidget->value();
    state.source.frequency = m_frequencyControlWidget->value();
    state.source.currentPhaseDegrees = m_currentPhaseDial->value();
    state.simulation.timeScale = m_timeScaleControlWidget->value();
    state.simulation.samplingCycles = m_samplingCyclesControlWidget->value();
    state.simulation.samplesPerCycle = static_cast<int>(m_samplesPerCycleControlWidget->value());

    if(m_perSampleRadioButton->isChecked()) {
        state.simulation.updateMode = UpdateMode::PerSample;
    } else if(m_perHalfCycleRadioButton->isChecked()) {
        state.simulation.updateMode = UpdateMode::PerHalfCycle;
    } else {
        state.simulation.updateMode = UpdateMode::PerCycle;
    }

    state.view.isRunning = (m_startStopButton->text() == "일시정지");

    return state;
}

void ControlPanel::setState(const ControlPanelState& state)
{
    // ControlPanelState 객체의 값으로 모든 자식 위젯들의 상태를 업데이트
    m_voltageControlWidget->setValue(state.source.amplitude);
    m_currentAmplitudeControlWidget->setValue(state.source.currentAmplitude);
    m_frequencyControlWidget->setValue(state.source.frequency);
    m_currentPhaseDial->setValue(state.source.currentPhaseDegrees);
    m_timeScaleControlWidget->setValue(state.simulation.timeScale);
    m_samplingCyclesControlWidget->setValue(state.simulation.samplingCycles);
    m_samplesPerCycleControlWidget->setValue(state.simulation.samplesPerCycle);

    if(state.simulation.updateMode == UpdateMode::PerSample) {
        m_perSampleRadioButton->setChecked(true);
    } else if(state.simulation.updateMode == UpdateMode::PerHalfCycle) {
        m_perHalfCycleRadioButton->setChecked(true);
    } else {
        m_perCycleRadioButton->setChecked(true);
    }

    setRunningState(state.view.isRunning);
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

    // 고조파 위젯
    m_harmonicsButton = new QPushButton("고조파 설정...");

    m_currentPhaseDial = new FineTuningDial();
    m_currentPhaseLabel = new QLabel();

    m_perSampleRadioButton = new QRadioButton("1 Sample");
    m_perHalfCycleRadioButton = new QRadioButton("Half Cycle");
    m_perCycleRadioButton = new QRadioButton("1 Cycle");

    m_autoScrollCheckBox = new QCheckBox("자동 스크롤");
    m_trackingButton = new QPushButton("자동 추적 시작", this);


    // 스크롤 영역 생성
    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 수평 스크롤 바는 항상 끔

    // 스크롤 영역에 들어갈 컨테이너 위젯 생성
    auto containerWidget = new QWidget();
    scrollArea->setWidget(containerWidget);

    // 레이아웃 설정
    auto mainLayout = new QVBoxLayout(containerWidget);

    // ControlPanel 최상위 레이아웃 설정
    auto topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(scrollArea);

    // 버튼 레이아웃
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    buttonLayout->addWidget(m_startStopButton);
    buttonLayout->addWidget(m_settingButton);
    buttonLayout->addWidget(m_trackingButton);
    buttonLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // 기본 파라미터 그룹
    auto parameterGroupBox = new QGroupBox("기본 파라미터");
    auto formLayout = new QFormLayout();
    formLayout->addRow("진폭", m_voltageControlWidget);
    formLayout->addRow("전류 진폭", m_currentAmplitudeControlWidget);
    formLayout->addRow("주파수", m_frequencyControlWidget);
    formLayout->addRow("시간 배율", m_timeScaleControlWidget);
    formLayout->addRow("초당 cycle", m_samplingCyclesControlWidget);
    formLayout->addRow("cycle당 sample", m_samplesPerCycleControlWidget);
    parameterGroupBox->setLayout(formLayout);
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

    // 그래프 표시 그룹박스 레이아웃
    m_waveformSelectionGroup = new CollapsibleGroupBox("waveform 그래프 표시 선택");
    QVBoxLayout* selectionLayout = m_waveformSelectionGroup->contentLayoutPtr();
    auto* waveformGridLayout = new QGridLayout();

    waveformGridLayout->addWidget(new QLabel("V"), 0, 1, Qt::AlignCenter);
    waveformGridLayout->addWidget(new QLabel("I"), 0, 2, Qt::AlignCenter);
    waveformGridLayout->addWidget(new QLabel("A상"), 1, 0, Qt::AlignCenter);
    waveformGridLayout->addWidget(new QLabel("B상"), 2, 0, Qt::AlignCenter);
    waveformGridLayout->addWidget(new QLabel("C상"), 3, 0, Qt::AlignCenter);

    // 체크 박스 생성 및 배치
    for(int phase{0}; phase < 3; ++phase) {
        m_voltageCheckBox[phase] = new QCheckBox();
        m_currentCheckBox[phase] = new QCheckBox();
        waveformGridLayout->addWidget(m_voltageCheckBox[phase], phase + 1, 1, Qt::AlignCenter);
        waveformGridLayout->addWidget(m_currentCheckBox[phase], phase + 1, 2, Qt::AlignCenter);
    }
    selectionLayout->addLayout(waveformGridLayout);

    // --- 분석 그래프 표시 그룹박스 ---
    m_analysisSelectionGroup = new CollapsibleGroupBox("Cycle 분석 그래프 선택");
    auto* analysisLayout = m_analysisSelectionGroup->contentLayoutPtr();
    auto* analysisGridLayout = new QGridLayout();

    analysisGridLayout->addWidget(new QLabel("V RMS"), 0, 1, Qt::AlignCenter);
    analysisGridLayout->addWidget(new QLabel("I RMS"), 0, 2, Qt::AlignCenter);
    analysisGridLayout->addWidget(new QLabel("Power"), 0, 3, Qt::AlignCenter);
    analysisGridLayout->addWidget(new QLabel("A상"), 1, 0, Qt::AlignCenter);
    analysisGridLayout->addWidget(new QLabel("B상"), 2, 0, Qt::AlignCenter);
    analysisGridLayout->addWidget(new QLabel("C상"), 3, 0, Qt::AlignCenter);

    // 3x3 체크박스 생성 및 배치
    for(int phase{0}; phase < 3; ++phase) {
        m_rmsVoltageCheckBox[phase] = new QCheckBox();
        m_rmsCurrentCheckBox[phase] = new QCheckBox();
        m_activePowerCheckBox[phase] = new QCheckBox();
        analysisGridLayout->addWidget(m_rmsVoltageCheckBox[phase], phase + 1, 1, Qt::AlignCenter);
        analysisGridLayout->addWidget(m_rmsCurrentCheckBox[phase], phase + 1, 2, Qt::AlignCenter);
        analysisGridLayout->addWidget(m_activePowerCheckBox[phase], phase + 1, 3, Qt::AlignCenter);
    }
    analysisLayout->addLayout(analysisGridLayout);

    //  --- Phasor 그래프 선택 그롭박스 ---
    m_phasorSelectionGroup = new CollapsibleGroupBox("Phasor 그래프 선택");
    auto* phasorLayout = m_phasorSelectionGroup->contentLayoutPtr();
    auto* phasorGridLayout = new QGridLayout();

    phasorGridLayout->addWidget(new QLabel("V"), 0, 1, Qt::AlignCenter);
    phasorGridLayout->addWidget(new QLabel("I"), 0, 2, Qt::AlignCenter);
    phasorGridLayout->addWidget(new QLabel("A상"), 1, 0, Qt::AlignCenter);
    phasorGridLayout->addWidget(new QLabel("B상"), 2, 0, Qt::AlignCenter);
    phasorGridLayout->addWidget(new QLabel("C상"), 3, 0, Qt::AlignCenter);
    phasorGridLayout->addWidget(new QLabel("고조파"), 4, 0, Qt::AlignCenter);

    for(int phase{0}; phase < 3; ++phase) {
        m_phasorFundVoltageCheckBox[phase] = new QCheckBox();
        m_phasorFundCurrentCheckBox[phase] = new QCheckBox();
        phasorGridLayout->addWidget(m_phasorFundVoltageCheckBox[phase], phase + 1, 1, Qt::AlignCenter);
        phasorGridLayout->addWidget(m_phasorFundCurrentCheckBox[phase], phase + 1, 2, Qt::AlignCenter);
    }
    m_phasorHarmVoltageCheckBox = new QCheckBox();
    m_phasorHarmCurrentCheckBox = new QCheckBox();
    phasorGridLayout->addWidget(m_phasorHarmVoltageCheckBox, 4, 1, Qt::AlignHCenter);
    phasorGridLayout->addWidget(m_phasorHarmCurrentCheckBox, 4, 2, Qt::AlignHCenter);
    phasorLayout->addLayout(phasorGridLayout);

    // 메인 레이아웃에 추가
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(parameterGroupBox);
    mainLayout->addWidget(phaseGroupBox);
    mainLayout->addWidget(m_harmonicsButton);
    mainLayout->addWidget(updateModeGroupBox);
    mainLayout->addWidget(m_waveformSelectionGroup);
    mainLayout->addWidget(m_analysisSelectionGroup);
    mainLayout->addWidget(m_phasorSelectionGroup);
    mainLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
}

void ControlPanel::setupWidgetProperties()
{
    m_voltageControlWidget->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    m_voltageControlWidget->setSuffix(" V");

    m_currentAmplitudeControlWidget->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    m_currentAmplitudeControlWidget->setSuffix(" A");

    m_frequencyControlWidget->setRange(config::Source::Frequency::Min, config::Source::Frequency::Max);
    m_frequencyControlWidget->setSuffix(" Hz");

    m_timeScaleControlWidget->setRange(config::TimeScale::Min, config::TimeScale::Max);
    m_timeScaleControlWidget->setSuffix(" x");

    m_samplingCyclesControlWidget->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    m_samplesPerCycleControlWidget->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    m_samplesPerCycleControlWidget->setDataType(ValueControlWidget::DataType::Integer);

    m_currentPhaseDial->setRange(0, 359);
    m_currentPhaseDial->setWrapping(true);
    m_currentPhaseDial->setNotchesVisible(true);
    updateCurrentPhaseLabel(m_currentPhaseDial->value());

    m_perSampleRadioButton->setChecked(true);
    m_autoScrollCheckBox->setChecked(true);

    // 그래프 표시 체크박스 초기화
    m_voltageCheckBox[0]->setChecked(true);
    m_currentCheckBox[0]->setChecked(true);

    // 분석 그래프 표시 체크박스 초기화
    m_rmsVoltageCheckBox[0]->setChecked(true);
    m_rmsCurrentCheckBox[0]->setChecked(true);
    m_activePowerCheckBox[0]->setChecked(true);

    // Phasor 그래프 표시 체크박스 초기화
    m_phasorFundVoltageCheckBox[0]->setChecked(true);
    m_phasorFundCurrentCheckBox[0]->setChecked(true);
}

void ControlPanel::createConnections()
{
    // 버튼 클릭 연결
    connect(m_startStopButton, &QPushButton::clicked, this, &ControlPanel::startStopClicked);
    connect(m_settingButton, &QPushButton::clicked, this, &ControlPanel::settingsClicked);
    connect(m_trackingButton, &QPushButton::clicked, this, [this]() {
        bool isTrackingNow = (m_trackingButton->text() == "자동 추적 중지");

        if(isTrackingNow) {
            m_trackingButton->setText("자동 추적 시작");
            emit trackingToggled(false);
        } else {
            m_trackingButton->setText("자동 추적 중지");
            emit trackingToggled(true);
        }
    });

    // 파라미터 변경
    connect(m_voltageControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::amplitudeChanged);
    connect(m_currentAmplitudeControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::currentAmplitudeChanged);
    connect(m_frequencyControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::frequencyChanged);
    connect(m_currentPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::currentPhaseChanged);
    connect(m_timeScaleControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::timeScaleChanged);
    connect(m_samplingCyclesControlWidget, &ValueControlWidget::valueChanged, this, &ControlPanel::samplingCyclesChanged);
    connect(m_samplesPerCycleControlWidget, &ValueControlWidget::intValueChanged, this, &ControlPanel::samplesPerCycleChanged);

    // 고조파 버튼 연결
    connect(m_harmonicsButton, &QPushButton::clicked, this, &ControlPanel::harmonicsSettingsRequested);

    // 라디오 버튼 연결
    connect(m_perSampleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perHalfCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);

    // 내부 UI 업데이트 연결
    connect(m_currentPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::updateCurrentPhaseLabel);

    // 체크박스 시그널 연결
    connect(m_autoScrollCheckBox, &QCheckBox::toggled, this, &ControlPanel::autoScrollToggled);

    // waveform, 분석, Phasor 그래프 체크박스 연결
    for(int phase{0}; phase < 3; ++phase) {
        // V 0, 2, 4
        connect(m_voltageCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool checked) {
            emit waveformVisibilityChanged(phase * 2 + 0, checked);
        });
        // I 1, 3, 5
        connect(m_currentCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool checked) {
            emit waveformVisibilityChanged(phase * 2 + 1, checked);
        });

        // V RMS 0, 3, 6
        connect(m_rmsVoltageCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool checked) {
            emit analysisWaveformVisibilityChanged(phase * 3 + 0, checked);
        });
        // I RMS 1, 4, 7
        connect(m_rmsCurrentCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool checked) {
            emit analysisWaveformVisibilityChanged(phase * 3 + 1, checked);
        });
        // activePower 2, 5, 8
        connect(m_activePowerCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool checked) {
            emit analysisWaveformVisibilityChanged(phase * 3 + 2, checked);
        });

        // 기본파 전압 0, 1, 2
        connect(m_phasorFundVoltageCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool c) {
            emit phasorVisibilityChanged(phase, c);
        });
        // 기본파 전류 3, 4, 5
        connect(m_phasorFundCurrentCheckBox[phase], &QCheckBox::toggled, this, [this, phase](bool c) {
            emit phasorVisibilityChanged(phase + 3, c);
        });
    }

    // 고조파 전압 6
    connect(m_phasorHarmVoltageCheckBox, &QCheckBox::toggled, this, [this](bool c) {
        emit phasorVisibilityChanged(6, c);
    });
    // 고조파 전압 7
    connect(m_phasorHarmCurrentCheckBox, &QCheckBox::toggled, this, [this](bool c) {
        emit phasorVisibilityChanged(7, c);
    });
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

void ControlPanel::setAmplitude(double value)
{
    QSignalBlocker blocker(m_voltageControlWidget);
    m_voltageControlWidget->setValue(value);
}
void ControlPanel::setCurrentAmplitude(double value)
{
    QSignalBlocker blocker(m_currentAmplitudeControlWidget);
    m_currentAmplitudeControlWidget->setValue(value);
}
void ControlPanel::setFrequency(double value)
{
    QSignalBlocker blocker(m_frequencyControlWidget);
    m_frequencyControlWidget->setValue(value);
}
void ControlPanel::setCurrentPhase(int degrees)
{
    QSignalBlocker blocker(m_currentPhaseDial);
    m_currentPhaseDial->setValue(degrees);
    updateCurrentPhaseLabel(degrees);
}
void ControlPanel::setTimeScale(double value)
{
    QSignalBlocker blocker(m_timeScaleControlWidget);
    m_timeScaleControlWidget->setValue(value);
}
void ControlPanel::setSamplingCycles(double value)
{
    QSignalBlocker blocker(m_samplingCyclesControlWidget);
    m_samplingCyclesControlWidget->setValue(value);
}
void ControlPanel::setSamplesPerCycle(int value)
{
    QSignalBlocker blocker(m_samplesPerCycleControlWidget);
    m_samplesPerCycleControlWidget->setValue(value);
}
void ControlPanel::setUpdateMode(UpdateMode mode)
{
    QSignalBlocker b1(m_perSampleRadioButton);
    QSignalBlocker b2(m_perHalfCycleRadioButton);
    QSignalBlocker b3(m_perCycleRadioButton);

    switch (mode) {
    case UpdateMode::PerSample: m_perSampleRadioButton->setChecked(true); break;
    case UpdateMode::PerHalfCycle: m_perHalfCycleRadioButton->setChecked(true); break;
    case UpdateMode::PerCycle: m_perCycleRadioButton->setChecked(true); break;
    }
}

// --------------------

// --- private slots ---
void ControlPanel::updateCurrentPhaseLabel(int value)
{
    m_currentPhaseLabel->setText(QString::number(value) + " °");
}
