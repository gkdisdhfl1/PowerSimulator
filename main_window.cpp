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

    // 자동 회전 타이머 설정
    m_autoRotateTimer.setInterval(16); // 약 60fps
    connect(&m_autoRotateTimer, &QTimer::timeout, this, &MainWindow::updateAutoRotation);


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


    // Auto 버튼 토글 시
    connect(ui->autoButton, &QPushButton::toggled, this, [this](bool checked) {
        if(checked) {
            ui->autoButton->setText("자동 회전 정지");
            ui->phaseDial->setEnabled(false);
            m_autoRotateTimer.start();
        } else {
            ui->autoButton->setText("자동 회전 시작");
            ui->phaseDial->setEnabled(true);
            m_autoRotateTimer.stop();
        }
    });

    // rpmEdit 입력이 끝나면, m_rotationSpeedHz 멤버 변수에 값을 저장
    connect(ui->rpmEdit, &QLineEdit::editingFinished, this, [this](){
        bool ok;
        double speed = ui->rpmEdit->text().toDouble(&ok);

        if(ok && speed >= 0) {
            m_rotationSpeedHz = speed;
            qDebug() << "Rotatoin speed set to: " << m_rotationSpeedHz << "Hz";
        } else {
            ui->rpmEdit->setText(QString::number(m_rotationSpeedHz));
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

void MainWindow::updateAutoRotation()
{
    if(m_rotationSpeedHz <= 0) return;

    // 1초당 회전할 각도
    double degreesPerSecond = m_rotationSpeedHz * 360.0;
    // 타이머의 현재 간격 만큼 이동할 각도
    double degreesPerInterval = degreesPerSecond * (m_autoRotateTimer.interval() / 1000.0);
    // 현재 다이얼 값에 계산된 각도를 더함
    double newDialValue = ui->phaseDial->value() + degreesPerInterval;

    newDialValue = std::fmod(newDialValue, 360);
    ui->phaseDial->setValue(static_cast<int>(newDialValue));
}
