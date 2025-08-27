#include "graph_window.h"
#include "config.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"

#include <QValueAxis>
#include <QLineSeries>
#include <QChart>
#include <QDebug>
#include <QGridLayout>
#include <ranges>

GraphWindow::GraphWindow(SimulationEngine* engine, QWidget *parent)
    : BaseGraphWindow(engine, parent)
    , m_voltageSeries(new QLineSeries(this)) // 부모를 지정하여 메모리 관리 위임
    , m_currentSeries(new QLineSeries(this))
    , m_axisY(new QValueAxis(this))
{
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
    connect(m_chartView, &CustomChartView::mouseMoved, this, &GraphWindow::findNearestPoint);

    // 그래프에 점 표시
    m_voltageSeries->setPointsVisible(true);
    m_currentSeries->setPointsVisible(true);

    setupSeries();
}

GraphWindow::~GraphWindow()
{
}

void GraphWindow::setupSeries()
{
    m_chart->setTitle(tr("실시간 전력 계측 시뮬레이션"));

    // 전압 시리즈 설정
    m_voltageSeries->setName("Voltage");
    m_chart->addSeries(m_voltageSeries);
    m_voltageSeries->setPointsVisible(true);

    // 전류 시리즈를 설정
    m_currentSeries->setName("Current");
    m_currentSeries->setColor(QColor("red"));
    m_chart->addSeries(m_currentSeries);
    m_currentSeries->setPointsVisible(true);

    // X축 설정
    m_axisX->setRange(0, m_engine->parameters().graphWidthSec ); // 초기 범위를 설정값으로
    m_voltageSeries->attachAxis(m_axisX);
    m_currentSeries->attachAxis(m_axisX);

    // Y축 설정
    m_axisY->setLabelFormat(tr("%.2f")); // 소수점 둘째 자리까지 V 단위로 표시
    m_axisY->setTitleText(tr("전압 (V/A)"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_voltageSeries->attachAxis(m_axisY);
    m_currentSeries->attachAxis(m_axisY);
}

void GraphWindow::stretchGraph(double factor)
{
    // if(!m_isAutoScrollEnabled)
    //     return;

    // 현재 그래프 폭에 팩터를 곱하여 새로운 폭을 계산
    m_engine->parameters().graphWidthSec /= factor;

    // 그래프 폭이 너무 크거나 작아지지 않도록 범위 제한
    m_engine->parameters().graphWidthSec = std::clamp(m_engine->parameters().graphWidthSec , config::View::GraphWidth::Min, config::View::GraphWidth::Max);

    // updateGraph를 즉시 호출하지 않음.
    qDebug() << "new graph width: " << m_engine->parameters().graphWidthSec  << "s";
}

void GraphWindow::updateGraph(const std::deque<DataPoint> &data)
{
    if (data.empty()) {
        m_voltageSeries->clear();
        m_currentSeries->clear();
        return;
    }

    updateVisiblePoints(data); // 현재 화면에 보일 데이터 포인터들만 필터링하여 멤버 변수에 저장
    updateSeriesData(); // 필터링된 데이터를 사용하여 그래프 시리즈의 내용을 교체
    updateAxes(data); // 자동 스크롤 모드일 경우, 축의 범위를 최신 데이터에 맞게 업데이트
}

void GraphWindow::findNearestPoint(const QPointF& chartPos)
{
    // 데이터 없으면 종료
    if(m_voltageSeries->points().isEmpty() || m_voltagePoints.isEmpty()) return;

    // 1. 이진 탐색으로 chartPos.x() 이상인 첫 번째 점을 찾음
    auto it = std::lower_bound(
        m_voltagePoints.begin(), m_voltagePoints.end(), chartPos.x(),
        [](const QPointF& p, double x) {
            return p.x() < x;
        });

    // 2. 후보군(it, it-1)을 벡터로 담음
    std::vector<const QPointF*> candidates;
    if(it != m_voltagePoints.end()) {
        candidates.push_back(&(*it));
    }
    if(it != m_voltagePoints.begin()) {
        candidates.push_back(&(*(it - 1)));
    }

    // 3. 스크린 좌표로 변환 & 거리 계산
    const QPointF mouseScreenPos = m_chartView->mapFromGlobal(QCursor::pos());
    const QPointF* nearestPoint = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for(const QPointF* c : candidates) {
        QPointF screenPos = m_chart->mapToPosition(*c, m_voltageSeries);
        double dist = QLineF(mouseScreenPos, screenPos).length();
        if(dist < minDistance) {
            minDistance = dist;
            nearestPoint = c;
        }
    }

    // 임계값 이내면 선택
    if(nearestPoint && minDistance <= config::View::Interaction::Proximity::Threshold) {
        emit pointHovered(*nearestPoint);
    }
}


void GraphWindow::updateVisiblePoints(const std::deque<DataPoint>& data)
{
    // 멤버 변수에 결과 저장
    m_voltagePoints.clear();
    m_currentPoints.clear();

    const auto [minX, maxX] = getVisibleXRange(data);

    // C++20 ranges를 사용해, 현재 보이는 X축 범위 내의 점들만 필터링하는 뷰를 생성
    auto pointsView = data
                      | std::views::transform([](const DataPoint& p) {
                            const auto x = utils::to_qpointf(p).x();
                            return std::make_pair(QPointF(x, p.voltage), QPointF(x, p.current));
                        })
                      | std::views::filter([minX, maxX](const auto& pair) {
                            return pair.first.x() >= minX && pair.first.x() <= maxX;
                        });


    for(const auto& pair : pointsView) {
        // qDebug() << "pointsview.size: " << std::ranges::distance(pointsView);
        m_voltagePoints.append(pair.first);
        m_currentPoints.append(pair.second);
    }
}

void GraphWindow::updateSeriesData()
{
    m_voltageSeries->replace(m_voltagePoints);
    m_currentSeries->replace(m_currentPoints);
}

void GraphWindow::updateAxes(const std::deque<DataPoint> &data)
{
    if(m_isAutoScrollEnabled) {
        if(m_voltagePoints.isEmpty() && m_currentPoints.isEmpty()) return;

        // Y값의 최소/최대값을 저장할 변수 초기화
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        updateMinMaxY(m_voltagePoints, minY, maxY);
        updateMinMaxY(m_currentPoints, minY, maxY);

        // Y축 범위 설정
        updateYAxisRange(minY, maxY);

        // X축의 범위를 업데이트
        const auto [minX, maxX] = getVisibleXRange(data);
        m_axisX->setRange(minX, maxX);
    }
}

void GraphWindow::updateMinMaxY(const QList<QPointF>& points, double& minY, double& maxY)
{
    if(points.isEmpty()) return;

    auto [min_it, max_it] = std::ranges::minmax_element(points, {}, &QPointF::y);
    minY = std::min(minY, min_it->y());
    maxY = std::max(maxY, max_it->y());
}

void GraphWindow::updateYAxisRange(double minY, double maxY)
{
    // 위아래 여백 계산
    double padding = (maxY - minY) * config::View::Padding::Ratio;
    if(padding < config::View::Padding::Min) {
        padding = config::View::Padding::Min; // 최소 여백 보장
    }

    m_axisY->setRange(minY - padding, maxY + padding);
}
