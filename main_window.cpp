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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_settingButton_clicked()
{
    qDebug() << "getMaxDataSize() = " << m_engine->getMaxDataSize();
    qDebug() << "getGraphWidth() = " << ui->graphViewPlaceholder->getGraphWidth();

    // 다이얼로그를 열기 전에 현재 설정값으로 초기화
    m_settingsDialog->setInitialValues(
        m_engine->getMaxDataSize(),
        ui->graphViewPlaceholder->getGraphWidth()
    );
    m_settingsDialog->open();
}

void MainWindow::onEngineRuninngStateChanged(bool isRunning)
{
    ui->startStopButton->setText(isRunning ? "일시정지" : "시작");
}



void MainWindow::setupUiWidgets()
{
    ui->voltageControlWidget->setRange(config::Amplitude::Min, config::Amplitude::Max);
    ui->voltageControlWidget->setValue(config::Amplitude::Default);

    ui->timeScaleWidget->setRange(config::TimeScale::Min, config::TimeScale::Max);
    ui->timeScaleWidget->setValue(config::TimeScale::Default);

    ui->frequencyControlWidget->setRange(config::Frequency::Min, config::Frequency::Max);
    ui->frequencyControlWidget->setValue(config::Frequency::Default);
}

void MainWindow::createSignalSlotConnections()
{
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


    // 설정 다이얼로그의 변경사항을 엔진과 그래프 윈도우에 각각 전달
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this,
            [this](double interval, int maxSize, double graphWidth) {
                m_engine->applySettings(interval, maxSize);
                ui->graphViewPlaceholder->setGraphWidth(graphWidth);
            });

    connect(ui->frequencyControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setFrequency);
    // ----------------------


    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, ui->graphViewPlaceholder, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, this, &MainWindow::onEngineRuninngStateChanged);
}
