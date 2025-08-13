#include "main_view.h"
#include "ui_main_view.h"
#include "graph_window.h"
#include "config.h"

MainView::MainView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainView)
{
    ui->setupUi(this);

    // --- 내부 위젯 시그널 -> MainView의 public 시그널로 연결 ---

    // 버튼 클릭 연결
    connect(ui->startStopButton, &QPushButton::clicked, this, &MainView::startStopClicked);
    connect(ui->settingButton, &QPushButton::clicked, this, &MainView::settingsClicked);

    // 파라미터 변경
    connect(ui->voltageControlWidget, &ValueControlWidget::valueChanged, this, &MainView::amplitudeChanged);
    connect(ui->currentAmplitudeControl, &ValueControlWidget::valueChanged, this, &MainView::currentAmplitudeChanged);
    connect(ui->frequencyControlWidget, &ValueControlWidget::valueChanged, this, &MainView::frequencyChanged);
    connect(ui->currentPhaseDial, &FineTuningDial::valueChanged, this, &MainView::currentPhaseChanged);
    connect(ui->timeScaleWidget, &ValueControlWidget::valueChanged, this, &MainView::timeScaleChanged);
    connect(ui->samplingCyclesControl, &ValueControlWidget::valueChanged, this, &MainView::samplingCyclesChanged);
    connect(ui->samplesPerCycleControl, &ValueControlWidget::valueChanged, this, [this](double value) {
        emit samplesPerCycleChanged(static_cast<int>(value));
    });

    // 라디오 버튼 연결
    connect(ui->perSampleRadioButton, &QRadioButton::toggled, this, &MainView::updateModeChanged);
    connect(ui->perHalfCycleRadioButton, &QRadioButton::toggled, this, &MainView::updateModeChanged);
    connect(ui->PerCycleRadioButton, &QRadioButton::toggled, this, &MainView::updateModeChanged);

    // 그래프 관련 연결
    connect(ui->autoScrollCheckBox, &QCheckBox::toggled, this, &MainView::autoScrollToggled);
    connect(ui->graphViewPlaceholder, &GraphWindow::pointHovered, this, &MainView::pointHovered);
    connect(ui->graphViewPlaceholder, &GraphWindow::redrawNeeded, this, &MainView::redrawNeeded);

    // 라벨 업데이트 같은 순수 UI 내부 동작은 여기서 처리
    connect(ui->currentPhaseDial, &FineTuningDial::valueChanged, this, [this](int value) {
        ui->currentPhaseLabel->setText(QString::number(value) + " °");
    });
}

MainView::~MainView()
{
    delete ui;
}

Ui::MainView* MainView::getUi() const
{
    return ui;
}

void MainView::initializeUiValues()
{
    ui->voltageControlWidget->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    ui->voltageControlWidget->setValue(config::Source::Amplitude::Default);
    ui->voltageControlWidget->setSuffix(" V");

    ui->currentAmplitudeControl->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    ui->currentAmplitudeControl->setValue(config::Source::Current::DefaultAmplitude);
    ui->currentAmplitudeControl->setSuffix(" A");

    ui->currentPhaseDial->setValue(config::Source::Current::DefaultPhaseOffset);

    ui->timeScaleWidget->setRange(config::TimeScale::Min, config::TimeScale::Max);
    ui->timeScaleWidget->setValue(config::TimeScale::Default);
    ui->timeScaleWidget->setSuffix(" x");

    ui->samplingCyclesControl->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    ui->samplingCyclesControl->setValue(config::Sampling::DefaultSamplingCycles);
    ui->samplesPerCycleControl->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    ui->samplesPerCycleControl->setValue(config::Sampling::DefaultSamplesPerCycle);

    ui->frequencyControlWidget->setRange(config::Source::Frequency::Min, config::Source::Frequency::Max);
    ui->frequencyControlWidget->setValue(config::Source::Frequency::Default);
    ui->frequencyControlWidget->setSuffix(" Hz");

    ui->currentPhaseDial->setValue(config::Source::Current::DefaultPhaseOffset);
    ui->currentPhaseLabel->setText(QString::number(ui->currentPhaseDial->value()) + " °");

    ui->perSampleRadioButton->setChecked(true);

}

void MainView::updateGraph(const std::deque<DataPoint>& data)
{
    ui->graphViewPlaceholder->updateGraph(data);
}

void MainView::setRunningState(bool isRunning)
{
    ui->startStopButton->setText(isRunning ? "일시정지" : "시작");
}

void MainView::setAutoScroll(bool enabled)
{
    ui->autoScrollCheckBox->setChecked(enabled);
}
