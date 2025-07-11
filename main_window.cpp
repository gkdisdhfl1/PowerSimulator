#include "main_window.h"
#include "./ui_main_window.h"
#include "config.h"
#include "graph_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "fine_tuning_dial.h"

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_graphWindow(new GraphWindow(this))
    , m_settingsDialog(new SettingsDialog(this))
    , m_engine(engine)
    , m_lastDialValue(0)
{
    ui->setupUi(this);
    m_graphWindow->show();

    // UI 초기값 설정
    ui->valueDial->setRange(config::DialMin, config::DialMax);
    ui->valueDial->setWrapping(true);
    ui->valueDial->setNotchesVisible(true);
    ui->valueSpinBox->setRange(config::MinVoltage, config::MaxVoltage);
    ui->valueSpinBox->setValue(config::DefaultVoltage);

    // --- 시그널/슬롯 연결 ---

    // UI 이벤트 -> SimulationEngine 슬롯
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });
    // connect(ui->valueDial, &QDial::sliderMoved, m_engine, &SimulationEngine::updateVoltage);
    connect(ui->valueDial, &QDial::sliderMoved, this, [this](int newDialValue) {
        int diff = newDialValue - m_lastDialValue;

        if(diff < -180)
            diff += 360;
        else if(diff > 180)
            diff -= 360;

        // 집중 모드 상태와 함께 계산된 변화량을 엔진에 전달
        m_engine->updateVoltage(diff, m_isFineTuningMode);

        m_lastDialValue = newDialValue;
    });
    connect(ui->valueSpinBox, &QDoubleSpinBox::editingFinished, this, [=, this]() {
        m_engine->setCurrentVoltage(ui->valueSpinBox->value());
    });
    // 커스텀 다이얼의 더블클릭 시그널을 상태 변경 슬롯에 연결
    connect(ui->valueDial, &FineTuningDial::doubleClicked, this, &MainWindow::toggleFineTuningMode);

    // 설정 다이얼로그의 변경사항을 엔진과 그래프 윈도우에 각각 전달
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this,
            [this](double interval, int maxSize, double graphWidth) {
        m_engine->applySettings(interval, maxSize);
        m_graphWindow->setGraphWidth(graphWidth);
    });

    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::statusChanged, ui->startStopButton, &QPushButton::setText);
    connect(m_engine, &SimulationEngine::voltageChanged, ui->valueSpinBox, &QDoubleSpinBox::setValue);

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

void MainWindow::toggleFineTuningMode()
{
    // bool 상태 반전
    m_isFineTuningMode = !m_isFineTuningMode;

    // UI 업데이트
    updateUiForTuningMode();
}

void MainWindow::updateUiForTuningMode()
{
    if(m_isFineTuningMode) {
        // 집중 모드
        // 전압 값 스핀박스의 배경색 변경
        ui->valueSpinBox->setStyleSheet("background-color: #e0f0ff;"); // 연한 파랑색
        qDebug() << "Fine-tuning mode: ON";
    } else {
        // 일반 모드
        ui->valueSpinBox->setStyleSheet(""); // 기본
        qDebug() << "Fine-tuning mode: OFF";
    }
}
