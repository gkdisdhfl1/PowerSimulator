#include "GraphWindow.h"
#include "ui_GraphWindow.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QGridLayout>
#include <QPointF>
#include <QVector>

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
    // ui->gridLayout->addWidget(m_chartView);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 레이아웃 설정
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(m_chartView);
    this->setLayout(mainLayout);


    m_series->setPointsVisible(true); // 그래프에 점 표시

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

void GraphWindow::updateGraph(const QList<QPointF>& points)
{
    if (points.isEmpty()) {
        m_series->clear();
        return;
    }

    // 시리즈의 데이터를 전달받은 포인트들로 한 번에 교체
    m_series->replace(points);

    // x, y축 범위를 데이터에 맞게 조절
    double minX = points.first().x();
    double maxX = points.last().x();

    double minY = points.first().y();
    double maxY = points.first().y();

    for (const auto& p : points) {
        if (p.y() < minY) minY = p.y();
        if (p.y() > maxY) maxY = p.y();
    }

    // 그래프가 위아래에 꽉 끼지 않도록 약간의 여백 줌
    // double y_padding = (maxY - minY) * 0.1;
    // if (y_padding < 5) y_padding = 5; // 최소 여백 확보

    // 계산된 범위로 축을 설정
    m_axisX->setRange(minX, maxX);
    // m_axisY->setRange(minY - y_padding, maxY + y_padding);

}
