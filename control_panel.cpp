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

    state.voltageHarmonic.order = static_cast<int>(m_voltageHarmonicOrder->value());
    state.voltageHarmonic.magnitude = m_voltageHarmonicMagnitude->value();
    state.voltageHarmonic.phase = m_voltageHarmonicPhaseDial->value();
    state.currentHarmonic.order = static_cast<int>(m_currentHarmonicOrder->value());
    state.currentHarmonic.magnitude = m_currentHarmonicMagnitude->value();
    state.currentHarmonic.phase = m_currentHarmonicPhaseDial->value();

    if(m_perSampleRadioButton->isChecked()) {
        state.updateMode = UpdateMode::PerSample;
    } else if(m_perHalfCycleRadioButton->isChecked()) {
        state.updateMode = UpdateMode::PerHalfCycle;
    } else {
        state.updateMode = UpdateMode::PerCycle;
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

    m_voltageHarmonicOrder->setValue(state.voltageHarmonic.order);
    m_voltageHarmonicMagnitude->setValue(state.voltageHarmonic.magnitude);
    m_voltageHarmonicPhaseDial->setValue(state.voltageHarmonic.phase);
    m_currentHarmonicOrder->setValue(state.currentHarmonic.order);
    m_currentHarmonicMagnitude->setValue(state.currentHarmonic.magnitude);
    m_currentHarmonicPhaseDial->setValue(state.currentHarmonic.phase);

    if(state.updateMode == UpdateMode::PerSample) {
        m_perSampleRadioButton->setChecked(true);
    } else if(state.updateMode == UpdateMode::PerHalfCycle) {
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

    // 고조파 위젯
    m_harmonicsTabWidget = new QTabWidget();
    m_voltageHarmonicOrder = new ValueControlWidget();
    m_voltageHarmonicMagnitude = new ValueControlWidget();
    m_voltageHarmonicPhaseDial= new FineTuningDial();
    m_voltageHarmonicPhaseLabel = new QLabel();
    m_currentHarmonicOrder = new ValueControlWidget();
    m_currentHarmonicMagnitude = new ValueControlWidget();
    m_currentHarmonicPhaseDial= new FineTuningDial();
    m_currentHarmonicPhaseLabel = new QLabel();

    m_currentPhaseDial = new FineTuningDial();
    m_currentPhaseLabel = new QLabel();

    m_perSampleRadioButton = new QRadioButton("1 Sample");
    m_perHalfCycleRadioButton = new QRadioButton("Half Cycle");
    m_perCycleRadioButton = new QRadioButton("1 Cycle");

    m_autoScrollCheckBox = new QCheckBox("자동 스크롤");
    m_trackingButton = new QPushButton("자동 추적 시작", this);

    m_waveformSelectionGroup = new CollapsibleGroupBox("그래프 표시 선택");
    m_voltageACheckBox = new QCheckBox();
    m_currentACheckBox = new QCheckBox();
    m_voltageBCheckBox = new QCheckBox();
    m_currentBCheckBox = new QCheckBox();
    m_voltageCCheckBox = new QCheckBox();
    m_currentCCheckBox = new QCheckBox();


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

    // ---- 고조파 탭 위젯 설정 ----
    auto voltageHarmonicTab = new QWidget();
    auto voltageHarmonicLayout = new QFormLayout(voltageHarmonicTab);

    // 전압 고조파 그룹
    voltageHarmonicLayout->addRow("차수", m_voltageHarmonicOrder);
    voltageHarmonicLayout->addRow("크기", m_voltageHarmonicMagnitude);
    auto voltageHarmonicPhaseLayout = new QHBoxLayout();
    voltageHarmonicPhaseLayout->addWidget(m_voltageHarmonicPhaseDial, 1);
    voltageHarmonicPhaseLayout->addWidget(m_voltageHarmonicPhaseLabel, 0, Qt::AlignHCenter);
    voltageHarmonicLayout->addRow("위상", voltageHarmonicPhaseLayout);

    // 전류 고조파 그룹
    auto currentHarmonicTab = new QWidget();
    auto currentHarmonicLayout = new QFormLayout(currentHarmonicTab);
    currentHarmonicLayout->addRow("차수", m_currentHarmonicOrder);
    currentHarmonicLayout->addRow("크기", m_currentHarmonicMagnitude);
    auto currentHarmonicPhaseLayout = new QHBoxLayout();
    currentHarmonicPhaseLayout->addWidget(m_currentHarmonicPhaseDial, 1);
    currentHarmonicPhaseLayout->addWidget(m_currentHarmonicPhaseLabel, 0, Qt::AlignHCenter);
    currentHarmonicLayout->addRow("위상", currentHarmonicPhaseLayout);

    m_harmonicsTabWidget->addTab(voltageHarmonicTab, "전압 고조파");
    m_harmonicsTabWidget->addTab(currentHarmonicTab, "전류 고조파");
    // --------------------------

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
    QVBoxLayout* selectionLayout = m_waveformSelectionGroup->contentLayoutPtr();

    auto *hLayout1 = new QHBoxLayout();
    m_voltageACheckBox = new QCheckBox("V(A)");
    m_voltageBCheckBox = new QCheckBox("V(B)");
    m_voltageCCheckBox = new QCheckBox("V(C)");
    hLayout1->addWidget(m_voltageACheckBox);
    hLayout1->addWidget(m_voltageBCheckBox);
    hLayout1->addWidget(m_voltageCCheckBox);
    hLayout1->addStretch();

    auto *hLayout2 = new QHBoxLayout();
    m_currentACheckBox = new QCheckBox("I(A)");
    m_currentBCheckBox = new QCheckBox("I(B)");
    m_currentCCheckBox = new QCheckBox("I(C)");
    hLayout2->addWidget(m_currentACheckBox);
    hLayout2->addWidget(m_currentBCheckBox);
    hLayout2->addWidget(m_currentCCheckBox);
    hLayout2->addStretch();

    selectionLayout->addLayout(hLayout1);
    selectionLayout->addLayout(hLayout2);

    // --- 분석 그래프 표시 그룹박스 ---
    m_analysisSelectionGroup = new CollapsibleGroupBox("Cycle 분석 그래프 선택");
    auto* analysisLayout = m_analysisSelectionGroup->contentLayoutPtr();
    auto* gridLayout = new QGridLayout();

    gridLayout->addWidget(new QLabel("V RMS"), 0, 1, Qt::AlignCenter);
    gridLayout->addWidget(new QLabel("I RMS"), 0, 2, Qt::AlignCenter);
    gridLayout->addWidget(new QLabel("Power"), 0, 3, Qt::AlignCenter);
    gridLayout->addWidget(new QLabel("A상"), 1, 0, Qt::AlignCenter);
    gridLayout->addWidget(new QLabel("B상"), 2, 0, Qt::AlignCenter);
    gridLayout->addWidget(new QLabel("C상"), 3, 0, Qt::AlignCenter);

    // 3x3 체크박스 생성 및 배치
    for(int phase{0}; phase < 3; ++phase) {
        m_rmsVoltageCheckBox[phase] = new QCheckBox();
        m_rmsCurrentCheckBox[phase] = new QCheckBox();
        m_activePowerCheckBox[phase] = new QCheckBox();
        gridLayout->addWidget(m_rmsVoltageCheckBox[phase], phase + 1, 1, Qt::AlignCenter);
        gridLayout->addWidget(m_rmsCurrentCheckBox[phase], phase + 1, 2, Qt::AlignCenter);
        gridLayout->addWidget(m_activePowerCheckBox[phase], phase + 1, 3, Qt::AlignCenter);
    }
    analysisLayout->addLayout(gridLayout);

    //  --- Phasor 그래프 선택 그롭박스 ---
    m_phasorSelectionGroup = new CollapsibleGroupBox("Phasor 그래프 선택");
    auto* phasorLayout = m_phasorSelectionGroup->contentLayoutPtr();
    auto* grid = new QGridLayout();

    // 위젯 이름과 체크 박스 배열을 묶어서 처리
    const QStringList v_labels = {"V(A)", "V(B)", "V(C)"};
    const QStringList i_labels = {"I(A)", "I(B)", "I(C)"};

    for(int i{0}; i < 3; ++i) {
        grid->addWidget(new QLabel(v_labels[i]), i, 0);
        grid->addWidget(m_phasorFundVoltageCheck[i] = new QCheckBox(), i, 1);
        grid->addWidget(new QLabel(i_labels[i]), i, 2);
        grid->addWidget(m_phasorFundCurrentCheck[i] = new QCheckBox(), i, 3);
    }

    grid->addWidget(new QLabel("고조파 V"), 0, 4);
    grid->addWidget(m_phasorHarmVoltageCheck = new QCheckBox(), 0, 5);
    grid->addWidget(new QLabel("고조파 I"), 1, 4);
    grid->addWidget(m_phasorHarmCurrentCheck = new QCheckBox(), 1, 5);

    phasorLayout->addLayout(grid);

    // 메인 레이아웃에 추가
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(parameterGroupBox);
    mainLayout->addWidget(phaseGroupBox);
    mainLayout->addWidget(m_harmonicsTabWidget);
    mainLayout->addWidget(updateModeGroupBox);
    mainLayout->addWidget(m_waveformSelectionGroup);
    mainLayout->addWidget(m_analysisSelectionGroup);
    mainLayout->addWidget(m_phasorSelectionGroup);
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

    // 전압 고조파 초기화
    m_voltageHarmonicOrder->setRange(1, 100);
    m_voltageHarmonicOrder->setValue(config::Harmonics::DefaultOrder);
    m_voltageHarmonicOrder->setDataType(ValueControlWidget::DataType::Integer);

    m_voltageHarmonicMagnitude->setRange(-config::Harmonics::MaxMangnitude, config::Harmonics::MaxMangnitude);
    m_voltageHarmonicMagnitude->setValue(config::Harmonics::DefaultMagnitude);
    m_voltageHarmonicMagnitude->setSuffix(" V");

    m_voltageHarmonicPhaseDial->setRange(0, 359);
    m_voltageHarmonicPhaseDial->setValue(config::Harmonics::DefaultPhase);
    m_voltageHarmonicPhaseDial->setWrapping(true);
    m_voltageHarmonicPhaseDial->setNotchesVisible(true);
    updateVoltageHarmonicPhaseLabel(m_voltageHarmonicPhaseDial->value());

    // 전류 고조파 초기화
    m_currentHarmonicOrder->setRange(1, 100);
    m_currentHarmonicOrder->setValue(config::Harmonics::DefaultOrder);
    m_currentHarmonicOrder->setDataType(ValueControlWidget::DataType::Integer);

    m_currentHarmonicMagnitude->setRange(-config::Harmonics::MaxMangnitude, config::Harmonics::MaxMangnitude);
    m_currentHarmonicMagnitude->setValue(config::Harmonics::DefaultMagnitude);
    m_currentHarmonicMagnitude->setSuffix(" A");

    m_currentHarmonicPhaseDial->setRange(0, 359);
    m_currentHarmonicPhaseDial->setValue(config::Harmonics::DefaultPhase);
    m_currentHarmonicPhaseDial->setWrapping(true);
    m_currentHarmonicPhaseDial->setNotchesVisible(true);
    updateCurrentHarmonicPhaseLabel(m_currentHarmonicPhaseDial->value());

    // 라벨 너비 고정
    QFontMetrics fm(m_currentPhaseLabel->font());
    int width = fm.horizontalAdvance("359 °"); // 가장 긴 텍스트의 너비 계산
    m_currentPhaseLabel->setMinimumWidth(width);
    m_voltageHarmonicPhaseLabel->setMinimumWidth(width);
    m_currentHarmonicPhaseLabel->setMinimumWidth(width);

    m_perSampleRadioButton->setChecked(true);
    m_autoScrollCheckBox->setChecked(true);

    // 그래프 표시 체크박스 초기화
    m_voltageACheckBox->setChecked(true);
    m_currentACheckBox->setChecked(true);
    m_voltageBCheckBox->setChecked(false);
    m_currentBCheckBox->setChecked(false);
    m_voltageCCheckBox->setChecked(false);
    m_currentCCheckBox->setChecked(false);

    // 분석 그래프 표시 체크박스 초기화
    m_rmsVoltageCheckBox[0]->setChecked(true);
    m_rmsCurrentCheckBox[0]->setChecked(true);
    m_activePowerCheckBox[0]->setChecked(true);

    // Phasor 그래프 표시 체크박스 초기화
    m_phasorFundVoltageCheck[0]->setChecked(true);
    m_phasorFundVoltageCheck[1]->setChecked(false);
    m_phasorFundVoltageCheck[2]->setChecked(false);
    m_phasorFundCurrentCheck[0]->setChecked(true);
    m_phasorFundCurrentCheck[1]->setChecked(false);
    m_phasorFundCurrentCheck[2]->setChecked(false);

    m_phasorHarmVoltageCheck->setChecked(false);
    m_phasorHarmCurrentCheck->setChecked(false);
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

    // 고조파 파리미터 변경
    connect(m_voltageHarmonicOrder, &ValueControlWidget::intValueChanged, this, &ControlPanel::harmonicChanged);
    connect(m_voltageHarmonicMagnitude, &ValueControlWidget::valueChanged, this, &ControlPanel::harmonicChanged);
    connect(m_voltageHarmonicPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::harmonicChanged);
    connect(m_currentHarmonicOrder, &ValueControlWidget::intValueChanged, this, &ControlPanel::harmonicChanged);
    connect(m_currentHarmonicMagnitude, &ValueControlWidget::valueChanged, this, &ControlPanel::harmonicChanged);
    connect(m_currentHarmonicPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::harmonicChanged);

    // 라디오 버튼 연결
    connect(m_perSampleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perHalfCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);
    connect(m_perCycleRadioButton, &QRadioButton::toggled, this, &ControlPanel::updateModeChanged);

    // 내부 UI 업데이트 연결
    connect(m_currentPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::updateCurrentPhaseLabel);
    connect(m_voltageHarmonicPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::updateVoltageHarmonicPhaseLabel);
    connect(m_currentHarmonicPhaseDial, &FineTuningDial::valueChanged, this, &ControlPanel::updateCurrentHarmonicPhaseLabel);

    // 체크박스 시그널 연결
    connect(m_autoScrollCheckBox, &QCheckBox::toggled, this, &ControlPanel::autoScrollToggled);
    connect(m_voltageACheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(0, checked);
    });
    connect(m_currentACheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(1, checked);
    });
    connect(m_voltageBCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(2, checked);
    });
    connect(m_currentBCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(3, checked);
    });
    connect(m_voltageCCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(4, checked);
    });
    connect(m_currentCCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit waveformVisibilityChanged(5, checked);
    });

    // 분석 그래프 체크박스 연결
    for(int phase{0}; phase < 3; ++phase) {
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
    }

    // Phasor 체크박스 시그널 연결
    for(int i{0}; i < 3; ++i) {
        // 기본파 전압 0, 1, 2
        connect(m_phasorFundVoltageCheck[i], &QCheckBox::toggled, this, [this, i](bool c) {
            emit phasorVisibilityChanged(i, c);
        });
        // 기본파 전류 3, 4, 5
        connect(m_phasorFundCurrentCheck[i], &QCheckBox::toggled, this, [this, i](bool c) {
            emit phasorVisibilityChanged(i + 3, c);
        });

        // 고조파 전압 6
        connect(m_phasorHarmVoltageCheck, &QCheckBox::toggled, this, [this](bool c) {
            emit phasorVisibilityChanged(6, c);
        });
        // 고조파 전압 7
        connect(m_phasorHarmCurrentCheck, &QCheckBox::toggled, this, [this](bool c) {
            emit phasorVisibilityChanged(7, c);
        });
    }
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

void ControlPanel::onEngineSamplingCyclesChanged(double newFrequency)
{
    // 이 슬롯은 엔진의 상태를 UI에 반영하는 역할
    // valueChanged 시그널이 다시 발생하여 로직이 꼬이면 안됨
    QSignalBlocker blocker(m_samplingCyclesControlWidget);
    m_samplingCyclesControlWidget->setValue(newFrequency);
}
// --------------------

// --- private slots ---
void ControlPanel::updateCurrentPhaseLabel(int value)
{
    m_currentPhaseLabel->setText(QString::number(value) + " °");
}

void ControlPanel::updateVoltageHarmonicPhaseLabel(int value)
{
    m_voltageHarmonicPhaseLabel->setText(QString::number(value) + " °");
}

void ControlPanel::updateCurrentHarmonicPhaseLabel(int value)
{
    m_currentHarmonicPhaseLabel->setText(QString::number(value) + " °");
}
