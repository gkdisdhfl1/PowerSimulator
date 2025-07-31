#include "main_window.h"
#include "./ui_main_window.h"
#include "config.h"
#include "settings_dialog.h"
#include "simulation_engine.h"

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settingsDialog(new SettingsDialog(this))
    , m_engine(engine)
{
    ui->setupUi(this);

    // UI 초기값 설정
    setupUiWidgets();
    createSignalSlotConnections();

    ui->splitter->setStretchFactor(0, 2);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleSettingButtonClicked()
{
    // qDebug() << "getMaxDataSize() = " << m_engine->getMaxDataSize();
    // qDebug() << "getGraphWidth() = " << ui->graphViewPlaceholder->getGraphWidth();

    // 다이얼로그를 열기 전에 현재 설정값으로 초기화
    m_settingsDialog->setInitialValues(
        m_engine->getMaxDataSize(),
        ui->graphViewPlaceholder->getGraphWidth()
    );

    if(m_settingsDialog->exec() == QDialog::Accepted) {
        // ok를 눌렀다면, 다이얼로그에서 새로운 값들을 가져와 적용
        m_engine->applySettings(
            m_settingsDialog->getMaxSize());
        ui->graphViewPlaceholder->setGraphWidth(m_settingsDialog->getGraphWidth());
    }
    // m_settingsDialog->open();
}

void MainWindow::onEngineRuninngStateChanged(bool isRunning)
{
    ui->startStopButton->setText(isRunning ? "일시정지" : "시작");
}



void MainWindow::setupUiWidgets()
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
}

void MainWindow::createSignalSlotConnections()
{
    connect(ui->settingButton, &QPushButton::clicked, this, &MainWindow::handleSettingButtonClicked);

    // ---- UI 이벤트 -> SimulationEngine 슬롯 ----
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });

    // voltageControlWidget의 값이 바뀌면, 엔진의 현재 전압을 설정
    connect(ui->voltageControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setAmplitude);

    // timeScaleWidget 값이 바뀌면 엔진의 setTimeScale 슬롯 호출
    connect(ui->timeScaleWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setTimeScale);

    connect(ui->samplingCyclesControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setSamplingCycles);
    connect(ui->samplesPerCycleControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setSamplesPerCycle);

    connect(ui->frequencyControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setFrequency);

    connect(ui->graphViewPlaceholder, &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);

    connect(ui->currentAmplitudeControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setCurrentAmplitude);
    connect(ui->currentPhaseDial, &FineTuningDial::valueChanged, this, [this](int value) {
        // 엔진에 값 전달
        m_engine->setCurrentPhaseOffset(value);
        // 라벨 업데이트
        ui->currentPhaseLabel->setText(QString::number(value) + " °");
    });

    // ----------------------


    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, ui->graphViewPlaceholder, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, this, &MainWindow::onEngineRuninngStateChanged);

    // 그래프 관련
    connect(ui->autoScrollCheckBox, &QCheckBox::toggled, ui->graphViewPlaceholder, &GraphWindow::toggleAutoScroll);
    connect(ui->graphViewPlaceholder, &GraphWindow::autoScrollToggled, ui->autoScrollCheckBox, &QCheckBox::setChecked);
    connect(ui->graphViewPlaceholder, &GraphWindow::pointHovered, this, [this](const QPointF& point) {
        std::string coordText = std::format("시간: {:.3f} s, 전압: {:.3f} V", point.x(), point.y());
        if(ui->statusbar)
            ui->statusbar->showMessage(QString::fromStdString(coordText));
    });
}
