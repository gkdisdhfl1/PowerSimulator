#include "main_window.h"
#include "fundamental_analysis_graph_window.h"
#include "harmonic_analysis_graph_window.h"
#include "one_second_summary_window.h"
#include "pid_tuning_dialog.h"
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
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , m_engine(engine)
    , m_fpsLabel(new QLabel("FPS: 0", this))
    , m_fpsTimer(new QTimer(this))
    , m_frameCount(0)
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
    statusBar()->addPermanentWidget(m_fpsLabel);
    m_fpsTimer->start(1000);
    resize(1500,870);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createMenus()
{
    // '파일' 메뉴 생성
    QMenu *fileMenu = menuBar()->addMenu("파일(&F)");

    // '설정' 액션 생성 및 메뉴에 추가
    m_actionSettings = new QAction("설정(&S)", this);
    fileMenu->addAction(m_actionSettings);

    // '도구' 메뉴 생성
    QMenu *toolsMenu = menuBar()->addMenu("도구(&T)");

    // 'PID 튜닝' 액션 생성 및 메뉴에 추가
    m_actionPidTuning = new QAction("PID 튜닝", this);
    toolsMenu->addAction(m_actionPidTuning);
}

void MainWindow::setupUiComponents()
{
    // 중앙 위젯을 사용하지 않도록 설정 (도킹 공간으로만 사용)
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

    // 기본파 RMS 그래프 창 도킹 위젯 생성
    m_fundamentalAnalysisGraphWindow = new FundamentalAnalysisGraphWindow(m_engine, this);
    QDockWidget *fundamentalRmsDock = new QDockWidget("fundamental Analysis", this);
    fundamentalRmsDock->setWidget(m_fundamentalAnalysisGraphWindow);

    // 고조파 RMS 그래프 창 도킹 위젯 생성
    m_harmonicAnalysisGraphWindow = new HarmonicAnalysisGraphWindow(m_engine, this) ;
    QDockWidget *harmonicRmsDock = new QDockWidget("Harmonic Analysis", this);
    harmonicRmsDock->setWidget(m_harmonicAnalysisGraphWindow);

    // 1초 데이터 창 추가
    m_oneSecondSummaryWindow = new OneSecondSummaryWindow(this);
    QDockWidget *oneSecondSummaryDock = new QDockWidget("1초 데이터", this);
    oneSecondSummaryDock->setWidget(m_oneSecondSummaryWindow);
    addDockWidget(Qt::RightDockWidgetArea, oneSecondSummaryDock);

    // 상하로 분할
    splitDockWidget(graphDock, analysisGraphDock, Qt::Vertical);

    // // 좌우로 분할
    splitDockWidget(analysisGraphDock, phasorDock, Qt::Horizontal);
    // splitDockWidget(phasorDock, analysisGraphDock, Qt::Horizontal);

    tabifyDockWidget(analysisGraphDock, fundamentalRmsDock);
    tabifyDockWidget(analysisGraphDock, harmonicRmsDock);
    analysisGraphDock->raise(); // 전체 RMS를 기본으로 선택

    // 하단 두 위젯 너비 비율 설정
    QList<int> bottomSizes;
    bottomSizes << 400 << 260;
    resizeDocks({analysisGraphDock, phasorDock}, bottomSizes, Qt::Horizontal);

    // 상단과 하단 영역의 높이 비율 설정
    QList<int> mainSizes;
    mainSizes << 500 << 250;
    resizeDocks({graphDock, analysisGraphDock}, mainSizes, Qt::Vertical);

    splitDockWidget(graphDock, oneSecondSummaryDock, Qt::Horizontal);
    QList<int> rightSizes;
    rightSizes << 600 << 200;
    resizeDocks({graphDock, oneSecondSummaryDock}, rightSizes, Qt::Horizontal);
}

void MainWindow::createSignalSlotConnections()
{
    // 메뉴바 액션 연결
    connect(m_actionSettings, &QAction::triggered, m_settingsUiController.get(), &SettingsUiController::showSettingsDialog);
    connect(m_actionPidTuning, &QAction::triggered, m_settingsUiController.get(), &SettingsUiController::showPidTuningDialog);
    connect(m_fpsTimer, &QTimer::timeout, this, &MainWindow::updateFpsLabel);

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
    connect(m_controlPanel, &ControlPanel::harmonicChanged, m_settingsUiController.get(), &SettingsUiController::onHarmonicsChanged);

    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_graphWindow, &GraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_analysisGraphWindow, &AnalysisGraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_fundamentalAnalysisGraphWindow, &FundamentalAnalysisGraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::autoScrollToggled, m_harmonicAnalysisGraphWindow, &HarmonicAnalysisGraphWindow::toggleAutoScroll);
    connect(m_controlPanel, &ControlPanel::trackingToggled, m_settingsUiController.get(), &SettingsUiController::onTrackingToggled);
    connect(m_controlPanel, &ControlPanel::waveformVisibilityChanged, m_graphWindow, &GraphWindow::onWaveformVisibilityChanged);
    // ----------------------

    // Model(engine) 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, m_controlPanel, &ControlPanel::setRunningState);
    connect(m_engine, &SimulationEngine::samplingCyclesUpdated, m_controlPanel, &ControlPanel::onEngineSamplingCyclesChanged);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_analysisGraphWindow, &AnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_phasorView, &PhasorView::updateData);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_fundamentalAnalysisGraphWindow, &FundamentalAnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::measuredDataUpdated, m_harmonicAnalysisGraphWindow, &HarmonicAnalysisGraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::oneSecondDataUpdated, m_oneSecondSummaryWindow, &OneSecondSummaryWindow::updateData);

    // ---- GraphWindow 시그널 -> UI 슬롯 ----
    connect(m_graphWindow, &GraphWindow::pointHovered, this, [this](const DataPoint& point) {
        const double timeSec = std::chrono::duration<double>(point.timestamp).count();
        QString status = QString::asprintf("Time: %.3f s, Voltage: %.3f V, Current: %.3f A",
                                           timeSec,
                                           point.voltage,
                                           point.current);
        statusBar()->showMessage(status);
    });
    connect(m_graphWindow, &GraphWindow::autoScrollToggled, m_controlPanel, &ControlPanel::setAutoScroll);
    connect(m_analysisGraphWindow, &AnalysisGraphWindow::autoScrollToggled, m_controlPanel, &ControlPanel::setAutoScroll);
    connect(m_fundamentalAnalysisGraphWindow, &FundamentalAnalysisGraphWindow::autoScrollToggled, m_controlPanel, &ControlPanel::setAutoScroll);
    // --------------------------

    // ---- GraphWindow 시그널 -> engine 슬롯
    connect(m_graphWindow, &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);
    connect(m_graphWindow, &GraphWindow::framePainted, this, [this](){
        ++m_frameCount;
    });
    connect(m_analysisGraphWindow, &AnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
    connect(m_fundamentalAnalysisGraphWindow, &FundamentalAnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
    connect(m_harmonicAnalysisGraphWindow, &HarmonicAnalysisGraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawAnalysisRequest);
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

void MainWindow::updateFpsLabel()
{
    // 1초간 누적도니 프레임 카운트를 라벨에 표시
    m_fpsLabel->setText(QString("FPS: %1").arg(m_frameCount));

    // 다음 1초를 위해 카운터 리셋
    m_frameCount = 0;
}
