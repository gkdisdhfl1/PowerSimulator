#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentVoltageValue(220.0)
    , m_lastDialValue(0)
{
    ui->setupUi(this);
    m_graphWindow = new GraphWindow(this);
    m_graphWindow->show();

    // connect(ui->valueDial, &QDial::valueChanged, this, &MainWindow::onDialValueChanged);
    // connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::onSpinBoxValueChanged);

    // 속성 설정
    ui->valueDial->setRange(0, 359); // 예: 0~400V
    ui->valueDial->setWrapping(true);
    ui->valueDial->setNotchesVisible(true);
    ui->valueSpinBox->setRange(-500.0, 500.0);

    // 다이얼 초기값 설정
    m_lastDialValue = ui->valueDial->value(); // 현재 다이얼 위치로 초기화
    ui->valueSpinBox->setValue(m_currentVoltageValue); // 스핀 박스에 초기 전압 표시

    connect(ui->valueDial, &QDial::sliderMoved, this, &MainWindow::onDialMoved);
    connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        m_currentVoltageValue = value;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_settingButton_clicked()
{
    SettingsDialog dialog(this);
    // dialog.setInitialValues(...); 나중에 추가할 부분

    // 이 부분도 나중에 추가
    // connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);
    dialog.exec();
}

void MainWindow::onDialMoved(int newDialValue)
{
    int diff = newDialValue - m_lastDialValue;

    // Wrapping 처리
    if(diff < -180) // 359 -> 0 으로 넘어갈 때(시계 방향)
        diff += 360;
    else if(diff > 180) // 0 -> 359 로 넘어갈 때(반시계 방향)
        diff -= 360;

    // 실제 전압 값 업데이트
    m_currentVoltageValue += static_cast<double>(diff);

    // 스핀박스에 반영
    bool oldState = ui->valueSpinBox->blockSignals(true);
    ui->valueSpinBox->setValue(m_currentVoltageValue);
    ui->valueSpinBox->blockSignals(oldState);

    // 다음 계산을 위해 현재 위치를 저장
    m_lastDialValue = newDialValue;
}
