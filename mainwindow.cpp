#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_graphWindow = new GraphWindow(this);
    m_graphWindow->show();

    connect(ui->valueDial, &QDial::valueChanged, this, &MainWindow::onDialValueChanged);
    connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::onSpinBoxValueChanged);

    // 초기값 설정
    ui->valueDial->setRange(0, 400); // 예: 0~400V
    ui->valueSpinBox->setRange(0.0, 400.0);
    ui->valueDial->setValue(220);
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

void MainWindow::onDialValueChanged(int value)
{
    // 스핀박스가 시그널을 보내지 못하도록 일시적으로 차단
    bool oldState = ui->valueSpinBox->blockSignals(true);

    ui->valueSpinBox->setValue(static_cast<double>(value)); // 값을 설정
    ui->valueSpinBox->blockSignals(oldState); // 차단 해제

    m_currentVoltageValue = static_cast<double>(value);
}

void MainWindow::onSpinBoxValueChanged(double value)
{
    // 다이얼이 시그널을 보내지 못하도록 일시적으로 차단
    bool oldState = ui->valueDial->blockSignals(true);

    ui->valueDial->setValue(static_cast<int>(round(value))); // 반올림 처리후 값을 설정
    ui->valueDial->blockSignals(oldState); // 차단 해제

    m_currentVoltageValue = static_cast<double>(value);
}

