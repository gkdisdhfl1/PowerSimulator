#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "graphwindow.h"
#include "settingsdialog.h"
#include "simulationengine.h"

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
    ui->valueDial->setRange(0, 359);
    ui->valueDial->setWrapping(true);
    ui->valueDial->setNotchesVisible(true);
    ui->valueSpinBox->setRange(-500.0, 500.0);
    ui->valueSpinBox->setValue(220.0);

    // --- 시그널/슬롯 연결 ---

    // UI 이벤트 -> SimulationEngine 슬롯
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });
    connect(ui->valueDial, &QDial::sliderMoved, m_engine, &SimulationEngine::updateVoltage);
    connect(ui->valueSpinBox, &QDoubleSpinBox::editingFinished, this, [=, this]() {
        m_engine->setCurrentVoltage(ui->valueSpinBox->value());
    });
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, m_engine, &SimulationEngine::applySettings);

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
        m_engine->getMaxDataSize()
    );
    m_settingsDialog->open();
}
