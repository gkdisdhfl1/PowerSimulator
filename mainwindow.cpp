#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "graphwindow.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QVector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentVoltageValue(220.0)
    , m_lastDialValue(0)
    , m_captureTimer(new QTimer(this))
    , m_elapsedTimer(new QElapsedTimer())
    , m_maxDataSize(100)
{
    ui->setupUi(this);

    m_captureTimer->setInterval(100); // 초기값은 100ms

    // GraphWindow 생성 및 표시
    m_graphWindow = new GraphWindow(this);
    m_graphWindow->show();

    // connect(ui->valueDial, &QDial::valueChanged, this, &MainWindow::onDialValueChanged);
    // connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::onSpinBoxValueChanged);

    // 속성 설정
    ui->valueDial->setRange(0, 359);
    ui->valueDial->setWrapping(true);
    ui->valueDial->setNotchesVisible(true);
    ui->valueSpinBox->setRange(-500.0, 500.0);

    // 다이얼 초기값 설정
    m_lastDialValue = ui->valueDial->value(); // 현재 다이얼 위치로 초기화
    ui->valueSpinBox->setValue(m_currentVoltageValue); // 스핀 박스에 초기 전압 표시

    // 연결
    connect(ui->valueDial, &QDial::sliderMoved, this, &MainWindow::onDialMoved);
    connect(ui->valueSpinBox, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        m_currentVoltageValue = value;
    });

    connect(m_captureTimer, &QTimer::timeout, this, &MainWindow::captureData);

    connect(this, &MainWindow::dataUpdated, m_graphWindow, &GraphWindow::updateGraph);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_settingButton_clicked()
{
    SettingsDialog dialog(this);

    // 다이얼 로그에 현재 설정값을 전달하여 초기화
    double currentInterval = m_captureTimer->interval() / 1000.0;
    dialog.setInitialValues(currentInterval, m_maxDataSize);

    // 다이얼로그 시그널을 MainWindow 슬릇에 연결
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);

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

    double nextVoltage = m_currentVoltageValue + static_cast<double>(diff);

    double minVoltage = ui->valueSpinBox->minimum();
    double maxVoltage = ui->valueSpinBox->maximum();

    // 실제 전압 값 업데이트
    m_currentVoltageValue = std::clamp(nextVoltage, minVoltage, maxVoltage);

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
        if(!m_graphWindow) {

        }


        // m_elapsedTimer->start();는 타이머가 한 번도 시작된 적 없을 때만 호출
        if (!m_elapsedTimer->isValid()) {
            m_elapsedTimer->start();
        }

        m_captureTimer->start();
        ui->startStopButton->setText("일시정지");
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
    if(!m_data.empty()) {
        emit dataUpdated(m_data);
    }
}

void MainWindow::applySettings(double interval, int maxSize)
{
    // 타이머 간격 업데이트
    m_captureTimer->setInterval(static_cast<int>(interval * 1000));

    // 크기 업데이트
    m_maxDataSize = maxSize;

    // deque 크기 재조정
    while(m_data.size() > static_cast<size_t>(m_maxDataSize))
        m_data.pop_front();

    qDebug() << "설정 반영 완료. Interval: " << interval << " Max Size: " << maxSize;
}
