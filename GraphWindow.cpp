#include "GraphWindow.h"
#include "ui_GraphWindow.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QGridLayout>
#include <QList>
#include <QPointF>

GraphWindow::GraphWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GraphWindow)
    , m_chart(new QChart())
    , m_series(new QLineSeries())
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_chartView(new QChartView(m_chart))
{
    ui->setupUi(this);

    m_chartView = new QChartView(m_chart);
    ui->gridLayout->addWidget(m_chartView);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    setupChart();
}

GraphWindow::~GraphWindow()
{
    delete ui;
}

void GraphWindow::setupChart()
{
    // 차트 기본 설정
    m_chart->setTitle("실시간 전력 계측 시뮬레이션");
    m_chart->legend()->hide(); // 범례는 숨김

    // 시리즈를 차트에 추가
    m_chart->addSeries(m_series);

    // X축 설정
    m_axisX->setLabelFormat("%.1f s"); // 소수점 첫째 자리까지 초 단위로 표시
    m_axisX->setTitleText("시간 (s)");
    m_axisX->setRange(0, 10);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    // Y축 설정
    m_axisY->setLabelFormat("%.2f V"); // 소수점 둘째 자리까지 V 단위로 표시
    m_axisY->setTitleText("전압 (V)");
    m_axisY->setRange(-500, 500);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
}

void GraphWindow::updateGraph(const QList<QPointF>& data)
{
    // 나중에 구현
    Q_UNUSED(data);
}
