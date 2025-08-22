#include "main_window.h"
#include "settings_dialog.h"
#include "ui_main_window.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include "settings_ui_controller.h"
#include "control_panel.h"
#include "graph_window.h"

#include <QDockWidget>
#include <QStatusBar>

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_engine(engine)
{
    ui->setupUi(this);

    // 데이터 베이스 설정
    QString dbPath = QApplication::applicationDirPath() + "/settings.db";
    m_settingsManager = std::make_unique<SettingsManager>(dbPath.toStdString());

    // UI 컴포넌트 생성 및 도킹
    setupUiComponents();

    // 컨트롤러 생성 (UI 컴포넌트가 생성된 후)
    m_settingsUiController = std::make_unique<SettingsUiController>(m_controlPanel, *m_settingsManager, m_engine, this);



    // 시그널-슬롯 연결
    createSignalSlotConnections();
    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUiComponents()
{
    // 중앙 위젯을 사용하지 않도록 설정 (도킹 공간으로만 사용)
    setCentralWidget(nullptr);

    // 컨트롤 패널 도킹 위젯 생성
    m_controlPanel = new ControlPanel(this);
    QDockWidget *controlDock = new QDockWidget("Control Panel", this);
    controlDock->setWidget(m_controlPanel);
    addDockWidget(Qt::LeftDockWidgetArea, controlDock);

    // 그래프 창 도킹 위젯 생성
    m_graphWindow = new GraphWindow(m_engine, this);
    QDockWidget *graphDock = new QDockWidget("Graph", this);
    graphDock->setWidget(m_graphWindow);
    addDockWidget(Qt::RightDockWidgetArea, graphDock);
}

void MainWindow::createSignalSlotConnections()
{
    // 메뉴바 액션 연결
    connect(ui->actionSettings, &QAction::triggered, m_settingsUiController.get(), &SettingsUiController::showSettingsDialog);

    // ---- ControlPanel 이벤트 -> Controller or Model(engine) 슬롯 ----
    connect(m_controlPanel, &ControlPanel::startStopClicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });
    connect(m_controlPanel, &ControlPanel::settingsClicked, m_settingsUiController.get(), &SettingsUiController::showSettingsDialog);

    connect(m_controlPanel, &ControlPanel::amplitudeChanged, m_settingsUiController.get(), &SettingsUiController::onAmplitudeChanged);
    connect(m_controlPanel, &ControlPanel::currentAmplitudeChanged, m_settingsUiController.get(), &SettingsUiController::onCurrentAmplitudeChanged);
    connect(m_controlPanel, &ControlPanel::frequencyChanged, m_settingsUiController.get(), &SettingsUiController::onFrequencyChanged);
    connect(m_controlPanel, &ControlPanel::currentPhaseChanged, m_settingsUiController.get(), &SettingsUiController::onCurrentPhaseChanged);
    connect(m_controlPanel, &ControlPanel::timeScaleChanged, m_settingsUiController.get(), &SettingsUiController::onTimeScaleChanged);
    connect(m_controlPanel, &ControlPanel::samplingCyclesChanged, m_settingsUiController.get(), &SettingsUiController::onSamplingCyclesChanged);
    connect(m_controlPanel, &ControlPanel::samplesPerCycleChanged, m_settingsUiController.get(), &SettingsUiController::onSamplesPerCycleChanged);
    connect(m_controlPanel, &ControlPanel::updateModeChanged, m_settingsUiController.get(), &SettingsUiController::onUpdateModeChanged);
    // ----------------------

    // Model(engine) 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, m_controlPanel, &ControlPanel::setRunningState);

    // ---- GraphWindow 시그널 -> UI 슬롯 ----
    connect(m_graphWindow, &GraphWindow::pointHovered, this, [this](const QPointF& point) {
        statusBar()->showMessage(QString("Time: %1 s, Voltage: %2 V").arg(point.x(), 0, 'f', 3).arg(point.y(), 0, 'f', 3));
    });
}
