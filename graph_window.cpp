#include "graph_window.h"
#include "config.h"
#include "ui_graph_window.h"

#include <QChartView>
#include <QDebug>
#include <QGridLayout>
#include <QLineSeries>
#include <QPointF>
#include <QValueAxis>
#include <QVector>

GraphWindow::GraphWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GraphWindow)
    , m_series(new QLineSeries(this)) // 부모를 지정하여 메모리 관리 위임
    , m_axisX(new QValueAxis(this))
    , m_axisY(new QValueAxis(this))
    , m_chartView(new QChartView(&m_chart))
    , m_graphWidthSec(config::DefaultGraphWidthSec) // 그래프 폭 기본값으로 초기화
{
    ui->setupUi(this);

    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 레이아웃 설정
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
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
    m_axisY->setRange(config::MinVoltage, config::MaxVoltage);
    m_chart.addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
}

void GraphWindow::updateGraph(const std::deque<DataPoint> &data)
{
    if (data.empty()) {
        m_series->clear();
        return;
    }

    // 최신 데이터 시간(그래프 오른쪽 끝)
    double maxX = data.back().timestampMs / 1000.0;
    // 가장 오래된 시간 (그래프 왼쪽 끝)
    double minX = maxX - m_graphWidthSec;

    QList<QPointF> visiblePoints;
    visiblePoints.reserve(data.size());
    // Y축 min/max 초기값을 첫 뎅터로 설정
    double minY = data.back().voltage;
    double maxY = data.back().voltage;

    // 역순 순회 및 데이터 추출
    for(auto it = data.rbegin(); it != data.rend(); ++it) {
        double currentX = it->timestampMs / 1000.0;

        // 보이는 범위를 벗어났는지 체크
        if(currentX < minX)
            break;

        // 보이는 점이므로 리스트에 추가
        visiblePoints.append(QPointF(currentX, it->voltage));

        // Y축 최소/최대값 실시간 업데이트
        if (it->voltage < minY) minY = it->voltage;
        if (it->voltage > maxY) maxY = it->voltage;
    }

    std::reverse(visiblePoints.begin(), visiblePoints.end());

    // 시리즈의 데이터를 전달받은 포인트들로 한 번에 교체
    m_series->replace(visiblePoints);

    // 그래프가 위아래에 꽉 끼지 않도록 약간의 여백 줌
    double y_padding = (maxY - minY) * 0.1;
    if (y_padding < 5)
        y_padding = 5; // 최소 여백 확보

    // 계산된 범위로 축을 설정
    m_axisX->setRange(minX, maxX);
    m_axisY->setRange(minY - y_padding, maxY + y_padding);
}
