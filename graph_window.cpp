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
    , m_chartView(new CustomChartView(m_chart.get()))
    , m_graphWidthSec(config::View::GraphWidth::Default) // 그래프 폭 기본값으로 초기화
    , m_isAutoScrollEnabled(true) // 자동 스크롤 활성화 상태로 시작
    , m_currentSeries(new QLineSeries(this))
    , m_axisYCurrent(new QValueAxis(this))
{
    ui->setupUi(this);

    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 레이아웃 설정
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_chartView);
    this->setLayout(mainLayout);

    // CustomChartView가 보내는 신호를 받아서 자동 스크롤을 끔
    connect(m_chartView, &CustomChartView::userInteracted, this, [this]() {
        if(m_isAutoScrollEnabled) {
            toggleAutoScroll(false);
            // MainWindow의 체크박스도 끄도록 시그널 보냄
            emit autoScrollToggled(false);
        }
        emit redrawNeeded();
    });

    connect(m_chartView, &CustomChartView::stretchRequested, this, &GraphWindow::stretchGraph);
    connect(m_chartView, &CustomChartView::mouseMoved, this, &GraphWindow::chartMouseMoved);
    connect(m_chartView, &CustomChartView::mouseMoved, this, &GraphWindow::findNearestPoint);

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

void GraphWindow::toggleAutoScroll(bool enabled)
{
    m_isAutoScrollEnabled = enabled;
}

void GraphWindow::setGraphWidth(double width)
{
    if (width > 0) {
        m_graphWidthSec = width;
        qDebug() << "그래프 폭 설정 완료. " << m_graphWidthSec << "s";
    }
}

void GraphWindow::stretchGraph(double factor)
{
    if(!m_isAutoScrollEnabled)
        return;

    // 현재 그래프 폭에 팩터를 곱하여 새로운 폭을 계산
    m_graphWidthSec /= factor;

    // 그래프 폭이 너무 크거나 작아지지 않도록 범위 제한
    m_graphWidthSec = std::clamp(m_graphWidthSec, config::View::GraphWidth::Min, config::View::GraphWidth::Max);

    // updateGraph를 즉시 호출하지 않음.
    qDebug() << "new graph width: " << m_graphWidthSec << "s";
}

void GraphWindow::setupChart()
{
    // 차트 기본 설정
    m_chart->setTitle(tr("실시간 전력 계측 시뮬레이션"));
    m_chart->legend()->show();

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
    m_axisY->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Min);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    // 전류 시리즈를 차트에 추가
    m_chart->addSeries(m_currentSeries);
    m_currentSeries->setName("Current");
    m_currentSeries->setPointsVisible(true);
    m_currentSeries->setColor(QColor("red"));

    // 전류 Y축 설정
    m_axisYCurrent->setLabelFormat(tr("%.2f A"));
    m_axisYCurrent->setTitleText(tr("전류 (A)"));
    m_chart->addAxis(m_axisYCurrent, Qt::AlignRight); // 차트 오른쪽에 축 추가
    m_currentSeries->attachAxis(m_axisX); // X축은 공유
    m_currentSeries->attachAxis(m_axisYCurrent);


}



void GraphWindow::updateGraph(const std::deque<DataPoint> &data)
{
    if (data.empty()) {
        m_series->clear();
        m_currentSeries->clear();
        return;
    }

    // using FpSeconds = std::chrono::duration<double>;

    double minX, maxX;
    auto *axisX = static_cast<QValueAxis*>(m_chart->axes(Qt::Horizontal).first());

    // 현재 모드에 따라 필터링할 X축의 범위를 결정
    if(m_isAutoScrollEnabled) {
        // 최신 데이터 시간(그래프 오른쪽 끝)
        maxX = utils::to_qpointf(data.back()).x();
        // 가장 오래된 시간 (그래프 왼쪽 끝)
        minX = maxX - m_graphWidthSec;
    } else {
        // 고정 줌 모드: 현재 축에 설정된 범위를 그대로 사용
        minX = axisX->min();
        maxX = axisX->max();
    }

    // 데이터처리 파이프라인
    // auto visiblePointsView = data
    //                            | std::views::transform(utils::to_qpointf) // DataPoint를 QPointF로 변환
    //                            | std::views::filter([minX, maxX](const QPointF& p) { return p.x() >= minX && p.x() <= maxX;}); // 보이는 점만 필터링

    auto pointsView = data
                      | std::views::transform([](const DataPoint& p) {
                            const auto x = utils::to_qpointf(p).x();
                            return std::make_pair(QPointF(x, p.voltage), QPointF(x, p.current));
                        })
                      | std::views::filter([minX, maxX](const auto& pair) {
                            return pair.first.x() >= minX && pair.first.x() <= maxX;
                        });

    // 전압/전류 포인트 분리
    QList<QPointF> voltagePoints;
    QList<QPointF> currentPoints;
    voltagePoints.reserve(data.size());
    currentPoints.reserve(data.size());

    for(const auto& pair : pointsView) {
        voltagePoints.append(pair.first);
        currentPoints.append(pair.second);
    }

    m_series->replace(voltagePoints);
    m_currentSeries->replace(currentPoints);
    m_currentPoints = voltagePoints; // 기존 로직을 유지하기 위해 전압 포인트 저장

    // 자동 스크롤이  활성화된 경우에만 축 범위를 업데이트
    if(m_isAutoScrollEnabled) {
        updateYAxisRange(m_axisY, voltagePoints); // 전압 축 업데이트
        updateYAxisRange(m_axisYCurrent, currentPoints); // 전류 측 업데이트
    }
}

void GraphWindow::findNearestPoint(const QPointF& chartPos)
{
    if(m_series->points().isEmpty()) {
        return;
    }

    auto it = std::lower_bound(m_currentPoints.begin(), m_currentPoints.end(), chartPos.x(),
                               [](const QPointF& p, double x) {
        return p.x() < x;
    });

    //찾은 위치와 그 이전 위치 중 더 가까운 점을 최종 후보로 선택
    if(it == m_currentPoints.begin()) {

    } else if(it == m_currentPoints.end()) {
        it = m_currentPoints.end() - 1;
    } else {
        if(std::abs((it - 1)->x() - chartPos.x()) < std::abs(it->x() - chartPos.x())) {
            --it;
        }
    }

    // 최종 선택된 점
    const QPointF& nearestPoint = *it;

    // 찾은 점이 커서와 화면상에서 너무 멀리 떨어져있는지 확인
    const QPointF nearestPointScreenPos = m_chart->mapToPosition(nearestPoint, m_series);
    const QPointF mouseScreenPos = m_chartView->mapFromGlobal(QCursor::pos());
    const double pixelDistance = QLineF(nearestPointScreenPos, mouseScreenPos).length();

    if(pixelDistance > config::View::Interaction::Proximity::Threshold)
        return;

    emit pointHovered(nearestPoint);
}

void GraphWindow::updateYAxisRange(QValueAxis *axis, const QList<QPointF> &points)
{
    if(!axis || points.isEmpty()) {
        return;
    }

    // Y값의 최소/최대 찾음
    auto [min_it, max_it] = std::ranges::minmax_element(points, {}, &QPointF::y);
    const double minY = min_it->y();
    const double maxY = max_it->y();

    // 위아래 여백 계산
    double padding = (maxY - minY) * config::View::Padding::Ratio;
    if(padding < config::View::Padding::Min) {
        padding = config::View::Padding::Min; // 최소 여백 보장
    }

    axis->setRange(minY - padding, maxY + padding);
}
