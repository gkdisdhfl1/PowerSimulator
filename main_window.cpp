#include "main_window.h"
#include "./ui_main_window.h"
#include "config.h"
#include "graph_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "value_control_widget.h"

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_graphWindow(new GraphWindow(this))
    , m_settingsDialog(new SettingsDialog(this))
    , m_engine(engine)
{
    ui->setupUi(this);
    m_graphWindow->show();

    // UI 초기값 설정
    ui->voltageControlWidget->setRange(config::MinVoltage, config::MaxVoltage);
    ui->voltageControlWidget->setValue(config::DefaultVoltage);

    // --- 시그널/슬롯 연결 ---

    // UI 이벤트 -> SimulationEngine 슬롯
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });

    // voltageControlWidget의 값이 바뀌면, 엔진의 현재 전압을 설정
    connect(ui->voltageControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setAmplitude);

    // phaseDial의 값이 바뀌면, 엔진의 위상을 설정
    connect(ui->phaseDial, &QDial::valueChanged, m_engine, &SimulationEngine::setPhase);

    // 설정 다이얼로그의 변경사항을 엔진과 그래프 윈도우에 각각 전달
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this,
            [this](double interval, int maxSize, double graphWidth) {
        m_engine->applySettings(interval, maxSize);
        m_graphWindow->setGraphWidth(graphWidth);
    });

    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::statusChanged, ui->startStopButton, &QPushButton::setText);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_settingButton_clicked()
{
    // 다이얼로그를 열기 전에 현재 설정값으로 초기화
    m_settingsDialog->setInitialValues(
        m_engine->getCaptureIntervalSec(),
        m_engine->getMaxDataSize(),
        m_graphWindow->getGraphWidth()
    );
    m_settingsDialog->open();
}
