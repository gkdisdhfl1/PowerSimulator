#include "graph_window.h"
#include "ui_graph_window.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QGridLayout>
#include <QPointF>
#include <QVector>

GraphWindow::GraphWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GraphWindow)
    , m_series(new QLineSeries())
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_chartView(new QChartView(&m_chart))
    , m_graphWidthSec(10.0) // 그래프 폭 기본값 10초로 초기화
{
    ui->setupUi(this);

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

double GraphWindow::getGraphWidth() const
{
    return m_graphWidthSec;
}

void GraphWindow::setGraphWidth(double width)
{
    if (width > 0) {
        m_graphWidthSec = width;
        qDebug() << "그래프 폭 설정 완료. " << m_graphWidthSec << "s";
    }
}

void GraphWindow::setupChart()
{
    // 차트 기본 설정
    m_chart.setTitle(tr("실시간 전력 계측 시뮬레이션"));
    m_chart.legend()->hide(); // 범례는 숨김

    // 시리즈를 차트에 추가
    m_chart.addSeries(m_series);

    // X축 설정
    m_axisX->setLabelFormat(tr("%.1f s")); // 소수점 첫째 자리까지 초 단위로 표시
    m_axisX->setTitleText(tr("시간 (s)"));
    m_axisX->setRange(0, m_graphWidthSec); // 초기 범위를 설정값으로
    m_chart.addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    // Y축 설정
    m_axisY->setLabelFormat(tr("%.2f V")); // 소수점 둘째 자리까지 V 단위로 표시
    m_axisY->setTitleText(tr("전압 (V)"));
    m_axisY->setRange(-500, 500);
    m_chart.addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
}

void GraphWindow::updateGraph(const std::deque<DataPoint>& data)
{
    if (data.empty()) {
        m_series->clear();
        return;
    }

    // DataPoint를 QPointF로 변환
    QList<QPointF> points;
    points.reserve(data.size()); // 미리 메모리 할당
    for(const auto& dp : data) {
        points.append(QPointF(dp.timestampMs / 1000.0, dp.voltage));
    }

    // 시리즈의 데이터를 전달받은 포인트들로 한 번에 교체
    m_series->replace(points);

    // Y축 범위 계산을 위해 현재 보이는 데이터만 필터링
    double maxX = points.last().x();
    double minX = maxX - m_graphWidthSec;

    double minY = points.last().y();
    double maxY = points.last().y();

    // 뒤에서부터 보이는 범위 내의 데이터만으로 min/max Y 계산
    for (auto it = points.rbegin(); it != points.rend(); ++it) {
        if (it->x() < minX) break; // 보이는 범위를 벗어나면 중단
        if (it->y() < minY) minY = it->y();
        if (it->y() > maxY) maxY = it->y();
    }

    // 그래프가 위아래에 꽉 끼지 않도록 약간의 여백 줌
    double y_padding = (maxY - minY) * 0.1;
    if (y_padding < 5) y_padding = 5; // 최소 여백 확보

    // 계산된 범위로 축을 설정
    m_axisX->setRange(minX, maxX);
    m_axisY->setRange(minY - y_padding, maxY + y_padding);
}
