#include "graph_window.h"
#include "config.h"
#include "ui_graph_window.h"

#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QChart>
#include <QDebug>
#include <QGridLayout>
#include <ranges>

GraphWindow::GraphWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GraphWindow)
    , m_chart(std::make_unique<QChart>())
    , m_series(new QLineSeries(this)) // 부모를 지정하여 메모리 관리 위임
    , m_axisX(new QValueAxis(this))
    , m_axisY(new QValueAxis(this))
    , m_chartView(new QChartView(m_chart.get()))
    , m_graphWidthSec(config::GraphWidth::Default) // 그래프 폭 기본값으로 초기화
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
    m_chart->setTitle(tr("실시간 전력 계측 시뮬레이션"));
    m_chart->legend()->hide(); // 범례는 숨김

    // 시리즈를 차트에 추가
    m_chart->addSeries(m_series);

    // X축 설정
    m_axisX->setLabelFormat(tr("%.1f s")); // 소수점 첫째 자리까지 초 단위로 표시
    m_axisX->setTitleText(tr("시간 (s)"));
    m_axisX->setRange(0, m_graphWidthSec); // 초기 범위를 설정값으로
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    // Y축 설정
    m_axisY->setLabelFormat(tr("%.2f V")); // 소수점 둘째 자리까지 V 단위로 표시
    m_axisY->setTitleText(tr("전압 (V)"));
    m_axisY->setRange(config::Amplitude::Min, config::Amplitude::Min);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
}

void GraphWindow::updateGraph(const std::deque<DataPoint> &data)
{
    if (data.empty()) {
        m_series->clear();
        return;
    }

    using FpSeconds = std::chrono::duration<double>;

    // 최신 데이터 시간(그래프 오른쪽 끝)
    const double maxX = utils::to_qpointf(data.back()).x();
    // 가장 오래된 시간 (그래프 왼쪽 끝)
    const double minX = maxX - m_graphWidthSec;

    // 데이터처리 파이프라인
    auto visiblePointsView = data
                               | std::views::transform(utils::to_qpointf) // DataPoint를 QPointF로 변환
                               | std::views::filter([minX](const QPointF& p) { return p.x() >= minX;}); // 보이는 점만 필터링

    QList<QPointF> pointsList;
    std::ranges::copy(visiblePointsView, std::back_inserter(pointsList));
    m_series->replace(pointsList);

    // Y축 범위 계산
    if(!pointsList.isEmpty()) {
        auto [minY_it, maxY_it] = std::ranges::minmax_element(pointsList, {}, &QPointF::y);
        double minY = minY_it->y();
        double maxY = maxY_it->y();

        // 그래프가 위아래에 꽉 끼지 않도록 약간의 여백 줌
        double y_padding = (maxY - minY) * 0.1;
        if (y_padding < 5)
            y_padding = 5; // 최소 여백 확보

        // 계산된 범위로 축을 설정
        m_axisY->setRange(minY - y_padding, maxY + y_padding);
    }
    m_axisX->setRange(minX, maxX);
}


