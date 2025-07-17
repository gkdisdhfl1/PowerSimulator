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

    ui->timeScaleWidget->setRange(1.0, 100.0);
    ui->timeScaleWidget->setValue(1.0);

    // UI의 초기 상태를 엔진에 반영
    m_engine->setFrequency(ui->rpmEdit->text().toDouble());
    m_engine->setPhase(ui->phaseDial->value());
    m_engine->setAutoRotation(ui->autoButton->isChecked());
    ui->phaseDial->setEnabled(!ui->autoButton->isChecked());
    if(ui->autoButton->isChecked())
        ui->autoButton->setText("자동 회전 정지");
    else
        ui->autoButton->setText("자동 회전 시작");


    // ---- 시그널/슬롯 연결 ----

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

    // phaseDial의 값이 바뀌면, 엔진의 위상을 설정
    connect(ui->phaseDial, &QDial::valueChanged, m_engine, &SimulationEngine::setPhase);
    connect(ui->phaseDial, &QDial::valueChanged, this, [this](int value){
        ui->phaseLabel->setText(QString::number(value) + "°");
    });

    // 설정 다이얼로그의 변경사항을 엔진과 그래프 윈도우에 각각 전달
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this,
            [this](double interval, int maxSize, double graphWidth) {
        m_engine->applySettings(interval, maxSize);
        m_graphWindow->setGraphWidth(graphWidth);
    });
    // ----------------------

    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::statusChanged, ui->startStopButton, &QPushButton::setText);

    // 엔진에서 위상이 업데이트되면 UI 다이얼과 라벨에 반영
    connect(m_engine, &SimulationEngine::phaseUpdated, this, [this](double newPhase){
        // 시그널 루프 방지
        ui->phaseDial->blockSignals(true);
        ui->phaseDial->setValue(static_cast<int>(newPhase));
        ui->phaseDial->blockSignals(false);
        ui->phaseLabel->setText(QString::number(static_cast<int>(newPhase)) + "°");
    });


    // Auto 버튼 토글 시
    connect(ui->autoButton, &QPushButton::toggled, this, [this](bool checked) {
        m_engine->setAutoRotation(checked);
        ui->phaseDial->setEnabled(!checked); // 자동회전 아닐때만 다이얼 활성화
        if(checked) {
            ui->autoButton->setText("자동 회전 정지");
        } else {
            ui->autoButton->setText("자동 회전 시작");
        }
    });

    // rpmEdit 입력이 끝나면, 엔진의 주파수를 설정
    connect(ui->rpmEdit, &QLineEdit::editingFinished, this, [this](){
        bool ok;
        double speed = ui->rpmEdit->text().toDouble(&ok);

        if(ok && speed >= 0) {
            m_engine->setFrequency(speed);
            qDebug() << "Rotatoin speed set to: " << speed << "Hz";
        } else {
            // 유효하지 않은 값이면, 현재 엔진의 주파수 값으로 UI를 되돌림
            // ui->rpmEdit->setText(QString::number(m_engine->getFrequency()));
        }
    });

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
