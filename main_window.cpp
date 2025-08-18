#include "main_window.h"
#include "ui_main_view.h"
#include "ui_main_window.h"
#include "main_view.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include "settings_ui_controller.h"

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_engine(engine)
{
    ui->setupUi(this);

    // 데이터 베이스 설정
    QString dbPath = QApplication::applicationDirPath() + "/settings.db";
    m_settingsManager = std::make_unique<SettingsManager>(dbPath.toStdString());

    m_view = new MainView(this);
    m_view->initializeUiValues();

    m_settingsUiController = std::make_unique<SettingsUiController>(m_view, *m_settingsManager, m_engine, this);

    // UI 조립
    setCentralWidget(m_view);
    statusBar()->showMessage("Ready");
    createSignalSlotConnections();

    // ui->splitter->setStretchFactor(0, 2);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onActionSettings()
{
    m_settingsUiController->showSettingsDialog();
}

void MainWindow::createSignalSlotConnections()
{
    // 메뉴바 액션 연결
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onActionSettings);

    // ---- View 이벤트 -> Controller or Model(engine) 슬롯 ----
    connect(m_view, &MainView::startStopClicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });
    connect(m_view, &MainView::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);
    connect(m_view, &MainView::settingsClicked, this, &MainWindow::onActionSettings);

    connect(m_view, &MainView::amplitudeChanged, m_settingsUiController.get(), &SettingsUiController::onAmplitudeChanged);
    connect(m_view, &MainView::currentAmplitudeChanged, m_settingsUiController.get(), &SettingsUiController::onCurrentAmplitudeChanged);
    connect(m_view, &MainView::frequencyChanged, m_settingsUiController.get(), &SettingsUiController::onFrequencyChanged);
    connect(m_view, &MainView::currentPhaseChanged, m_settingsUiController.get(), &SettingsUiController::onCurrentPhaseChanged);
    connect(m_view, &MainView::timeScaleChanged, m_settingsUiController.get(), &SettingsUiController::onTimeScaleChanged);
    connect(m_view, &MainView::samplingCyclesChanged, m_settingsUiController.get(), &SettingsUiController::onSamplingCyclesChanged);
    connect(m_view, &MainView::samplesPerCycleChanged, m_settingsUiController.get(), &SettingsUiController::onSamplesPerCycleChanged);
    connect(m_view, &MainView::updateModeChanged, m_settingsUiController.get(), &SettingsUiController::onUpdateModeChanged);
    // ----------------------

    // Model(engine) 시그널 -> View UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_view, &MainView::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, m_view, &MainView::setRunningState);

    // View의 내부 상호작용 (Graph -> CheckBox)
    connect(m_view->getUi()->graphViewPlaceholder, &GraphWindow::autoScrollToggled, m_view, &MainView::setAutoScroll);
    connect(m_view, &MainView::autoScrollToggled, m_view->getUi()->graphViewPlaceholder, &GraphWindow::toggleAutoScroll);

    // Model(engine) 데이터 변경 시그널 -> MainWindow 상태바 업데이트
    connect(m_view, &MainView::pointHovered, this, [this](const QPointF& point) {
        statusBar()->showMessage(QString("Time: %1 s, Voltage:/chat %2 V").arg(point.x(), 0, 'f', 3).arg(point.y(), 0, 'f', 3));
    });

}
