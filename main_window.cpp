#include "main_window.h"
#include "./ui_main_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include "settings_ui_controller.h"
#include "config.h"

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_engine(engine)
{
    ui->setupUi(this);

    // 데이터 베이스 설정
    QString dbPath = QApplication::applicationDirPath() + "/settings.db";
    m_settingsManager = std::make_unique<SettingsManager>(dbPath.toStdString());

    // 컨트롤러 객체 생성
    m_settingsUiController = std::make_unique<SettingsUiController>(ui, *m_settingsManager, m_engine, this);

    // UI 초기값 설정
    setupUiWidgets();
    createSignalSlotConnections();

    ui->splitter->setStretchFactor(0, 2);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onEngineRuninngStateChanged(bool isRunning)
{
    ui->startStopButton->setText(isRunning ? "일시정지" : "시작");
}

void MainWindow::onActionSaveSettings()
{
    m_settingsUiController->handleSaveAction();
}

void MainWindow::onActionLoadSettings()
{
    m_settingsUiController->handleLoadAction();
}

void MainWindow::onActionDeleteSettings()
{
    m_settingsUiController->handleDeleteAction();
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

    ui->perSampleRadioButton->setChecked(true);
}

void MainWindow::createSignalSlotConnections()
{
    connect(ui->settingButton, &QPushButton::clicked, m_settingsUiController.get(), &SettingsUiController::handleSettingsDialog);

    // 메뉴바 액션 연결
    connect(ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::onActionSaveSettings);
    connect(ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::onActionLoadSettings);
    connect(ui->actionDeleteSettings, &QAction::triggered, this, &MainWindow::onActionDeleteSettings);

    // ---- UI 이벤트 -> SimulationEngine 슬롯 ----
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
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
    connect(ui->graphViewPlaceholder, &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);

    connect(ui->currentPhaseDial, &FineTuningDial::valueChanged, this, [this](int value) {
        // 라벨 업데이트
        ui->currentPhaseLabel->setText(QString::number(value) + " °");
    });

}
