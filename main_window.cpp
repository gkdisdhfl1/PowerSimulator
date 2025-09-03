#include "main_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include "settings_ui_controller.h"
#include "control_panel.h"
#include "graph_window.h"
#include "analysis_graph_window.h"
#include "phasor_view.h"

#include <QDockWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QApplication>

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , m_engine(engine)
{
    // 메뉴바 생성
    createMenus();

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
    resize(1200,720);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createMenus()
{
    // '파일' 메뉴 생성
    QMenu *fileMenu = menuBar()->addMenu("파일(&F");

    // '설정' 액션 생성 및 메뉴에 추가
    m_actionSettings = new QAction("설정(&S)", this);
    fileMenu->addAction(m_actionSettings);
}

void MainWindow::setupUiComponents()
{
    // 중앙 위젯을 사용하지 않도록 설정 (도킹 공간으로만 사용)
    // QWidget* dummyCentral = new QWidget();
    // dummyCentral->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    // setCentralWidget(dummyCentral);
    setCentralWidget(nullptr);

    setDockNestingEnabled(true);

    // 컨트롤 패널 도킹 위젯 생성
    m_controlPanel = new ControlPanel(this);
    QDockWidget *controlDock = new QDockWidget("Control Panel", this);
    m_controlDock = controlDock;
    controlDock->setWidget(m_controlPanel);
    controlDock->setMinimumWidth(200);
    controlDock->setMaximumWidth(375);
    addDockWidget(Qt::LeftDockWidgetArea, controlDock);

    // 왼쪽 도킹 영역 유지를 위한 플레이스홀더 생성
    m_placeholderDock = new QDockWidget(this);
    m_placeholderDock->setWidget(new QWidget());
    m_placeholderDock->setTitleBarWidget(new QWidget());
    m_placeholderDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_placeholderDock->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX); // 최대 크기 제한 해제
    m_placeholderDock->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); // 크기 정책 변경
    m_placeholderDock->setMaximumWidth(0); // 너비를 0으로 고정
    addDockWidget(Qt::LeftDockWidgetArea, m_placeholderDock);
    m_placeholderDock->hide();

    // 컨트롤 패널의 상태 변경 시 시그널과 슬롯 연결
    connect(m_controlDock, &QDockWidget::dockLocationChanged, this, &MainWindow::updatePlaceholderVisibility);
    connect(m_controlDock, &QDockWidget::topLevelChanged, this, &MainWindow::updatePlaceholderVisibility);


    // 그래프 창 도킹 위젯 생성
    m_graphWindow = new GraphWindow(m_engine, this);
    QDockWidget *graphDock = new QDockWidget("Real-time Waveform", this);
    graphDock->setWidget(m_graphWindow);
    addDockWidget(Qt::RightDockWidgetArea, graphDock);

    // 분석 그래프 창 도킹 위젯 생성
    m_analysisGraphWindow = new AnalysisGraphWindow(m_engine, this);
    QDockWidget *analysisGraphDock = new QDockWidget("Cycle Analysis", this);
    analysisGraphDock->setWidget(m_analysisGraphWindow);

    // PhasorView 위젯 생성 및 도킹
    m_phasorView = new PhasorView(this);
    QDockWidget *phasorDock = new QDockWidget("Phasor", this);
    phasorDock->setWidget(m_phasorView);

    // 상하로 분할
    splitDockWidget(graphDock, analysisGraphDock, Qt::Vertical);

    // // 좌우로 분할
    splitDockWidget(analysisGraphDock, phasorDock, Qt::Horizontal);
    // splitDockWidget(phasorDock, analysisGraphDock, Qt::Horizontal);

    // 하단 두 위젯 너비 비율 설정
    QList<int> bottomSizes;
    bottomSizes << 400 << 260;
    resizeDocks({analysisGraphDock, phasorDock}, bottomSizes, Qt::Horizontal);

    // 상단과 하단 영역의 높이 비율 설정
    QList<int> mainSizes;
    mainSizes << 500 << 250;
    resizeDocks({graphDock, analysisGraphDock}, mainSizes, Qt::Vertical);
}

void MainWindow::createSignalSlotConnections()
{
    // 메뉴바 액션 연결
    connect(m_actionSettings, &QAction::triggered, m_settingsUiController.get(), &SettingsUiController::showSettingsDialog);

    connect(m_settingsUiController.get(), &SettingsUiController::maxDataSizeChangeRequested, m_engine, &SimulationEngine::onMaxDataSizeChanged);

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

    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_graphWindow, &GraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_analysisGraphWindow, &AnalysisGraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::trackingToggled, m_settingsUiController.get(), &SettingsUiController::onTrackingToggled);
    // ----------------------

    // Model(engine) 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, m_controlPanel, &ControlPanel::setRunningState);
    connect(m_engine, &SimulationEngine::samplingCyclesUpdated, m_controlPanel, &ControlPanel::onEngineSamplingCyclesChanged);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_analysisGraphWindow, &AnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_phasorView, &PhasorView::updateData);

    // ---- GraphWindow 시그널 -> UI 슬롯 ----
    connect(m_graphWindow, &GraphWindow::pointHovered, this, [this](const QPointF& point) {
        statusBar()->showMessage(QString("Time: %1 s, Voltage: %2 V").arg(point.x(), 0, 'f', 3).arg(point.y(), 0, 'f', 3));
    });
    connect(m_graphWindow, &GraphWindow::autoScrollToggled, m_controlPanel, &ControlPanel::setAutoScroll);
    connect(m_analysisGraphWindow, &AnalysisGraphWindow::autoScrollToggled, m_controlPanel, &ControlPanel::setAutoScroll);
    // --------------------------

    // ---- GraphWindow 시그널 -> engine 슬롯
    connect(m_graphWindow, &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);
    connect(m_analysisGraphWindow, &AnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
}

void MainWindow::updatePlaceholderVisibility()
{
    // 컨트롤 패널이 부유 상태이거나, 왼쪽 영역에 도킹되어 있지 않다면
    if (m_controlDock->isFloating() || dockWidgetArea(m_controlDock) != Qt::LeftDockWidgetArea) {
        // 왼쪽 영역을 유지하고, 드롭을 감지할 수 있도록 최소한의 너비를 설정
        m_placeholderDock->setMinimumWidth(10); // 10px 정도면 충분히 감지
        m_placeholderDock->show();
    } else {
        // 컨트롤 패널이 왼쪽 영역에 제대로 도킹되었다면 숨기고 너비도 0으로 만들어 공간 차지를 없앰
        m_placeholderDock->hide();
        m_placeholderDock->setMinimumWidth(0);
    }
}
