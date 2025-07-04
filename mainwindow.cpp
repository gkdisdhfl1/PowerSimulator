#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentVoltageValue(220.0)
    , m_lastDialValue(0)
    , m_captureTimer(new QTimer(this))
    , m_elapsedTimer(new QElapsedTimer())
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

    connect(m_captureTimer, &QTimer::timeout, this, &MainWindow::captureData);
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

void MainWindow::on_startStopButton_clicked()
{
    if(m_captureTimer->isActive()) {
        m_captureTimer->stop();
        ui->startStopButton->setText("시작");
        qDebug() << "Timer stopped";
    } else {
        // m_data.clear();
        // double interval = ui->intervalSpinBox->value() * 1000; // 나중에 설정창에서
        m_captureTimer->setInterval(100); // 우선 100ms로 고정
        m_elapsedTimer->start();
        m_captureTimer->start();
        ui->startStopButton->setText("중지");
        qDebug() << "Timer started.";
    }
}

void MainWindow::captureData()
{
    // 데이터 생성
    qint64 currentTimeMs = m_elapsedTimer->elapsed();
    double currentVoltage = m_currentVoltageValue;

    // 노이즈 추가
    // double noise = (qrand() % 20 - 10) / 20.0; // -0.5 ~ +0.5
    // currentVoltage += noise;

    // DataPoint 객체를 생성하여 저장
    m_data.push_back(DataPoint{currentTimeMs, currentVoltage});

    // 최대 개수 관리
    if(m_data.size() > m_maxDataSize) {
        qDebug() << m_data.front().timestampMs << "ms, "
                 << m_data.front().voltage << "V 가 삭제될 예정";
        m_data.pop_front();
    }

    // 디버그 출력으로 확인
    if(!m_data.empty()) {
        const auto& lastPoint = m_data.back();
        qDebug() << "Data captured: "
                 << lastPoint.timestampMs << "ms, "
                 << lastPoint.voltage << "V. Total points: "
                 << m_data.size();
    }

    // 그래프 업데이트 신호 발생
    // DataPoint를 QPointF로 변환하여 시그널 발생
    // Qlist ...

}
