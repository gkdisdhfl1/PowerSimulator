#include "graph_window.h"
#include "config.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"

#include <QValueAxis>
#include <QLineSeries>
#include <QChart>
#include <QDebug>
#include <QGridLayout>

using utils::FpSeconds;
using utils::Nanoseconds;

GraphWindow::GraphWindow(SimulationEngine* engine, QWidget *parent)
    : BaseGraphWindow(engine, parent)
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
    connect(m_chartView, &CustomChartView::framePainted, this, &GraphWindow::framePainted);

    setupSeries();
}

GraphWindow::~GraphWindow()
{
}

void GraphWindow::setupSeries()
{
    m_chart->setTitle(tr("실시간 전력 계측 시뮬레이션"));

    // Y축 설정
    m_axisY->setLabelFormat(tr("%.2f")); // 소수점 둘째 자리까지 V 단위로 표시
    m_axisY->setTitleText(tr("전압 (V/A)"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    // --- m_seriesInfoList 초기화---
    // A상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().voltage.a;},
        m_axisY,
        true, {}
    });
    m_seriesInfoList.back().series->setName("Voltage");

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().current.a;},
        m_axisY,
        true, {}
    });
    m_seriesInfoList.back().series->setName("Current");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::red));

    // B상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().voltage.b;},
        m_axisY,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Voltage B");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::yellow));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().current.b;},
        m_axisY,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Current B");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::orange));

    // C상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().voltage.c;},
        m_axisY,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Voltage C");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::black));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<DataPoint>().current.c;},
        m_axisY,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Current C");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::gray));

    // ----------------------------

    // X축 설정
    m_axisX->setRange(0, m_engine->parameters().graphWidthSec ); // 초기 범위를 설정값으로
    for(const auto& info : m_seriesInfoList) {
        m_chart->addSeries(info.series);
        info.series->attachAxis(m_axisX);
        info.series->attachAxis(info.yAxis);
        info.series->setVisible(info.isVisible);
        info.series->setPointsVisible(true);
    }

}

// --- public slot ----
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
        for(const auto& info : m_seriesInfoList) {
            info.series->clear();
        }
        return;
    }

    updateVisiblePoints(data); // 현재 화면에 보일 데이터 포인터들만 필터링하여 멤버 변수에 저장
    updateSeriesData(); // 필터링된 데이터를 사용하여 그래프 시리즈의 내용을 교체
    updateAxes(data); // 자동 스크롤 모드일 경우, 축의 범위를 최신 데이터에 맞게 업데이트
}

void GraphWindow::findNearestPoint(const QPointF& chartPos)
{
    // 데이터 없으면 종료
    if(m_visibleDataPoints.empty()) return;

    // 1. 이진 탐색으로 chartPos.x() 이상인 첫 번째 점을 찾음
    auto it = std::lower_bound(m_visibleDataPoints.begin(), m_visibleDataPoints.end(), chartPos.x(),
                               [&](const DataPoint& p, double x) {
                                   const double timeSec = FpSeconds(p.timestamp).count();
                                   return timeSec < x;
                               });

    // 2. 후보군(it, it-1)을 벡터로 담음
    std::vector<const DataPoint*> candidates;
    if(it != m_visibleDataPoints.end()) {
        candidates.push_back(&(*it));
    }
    if(it != m_visibleDataPoints.begin()) {
        candidates.push_back(&(*(it - 1)));
    }

    // 3. 스크린 좌표로 변환 & 거리 계산
    const QPointF mouseScreenPos = m_chartView->mapFromGlobal(QCursor::pos());
    const DataPoint* nearestPoint = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for(const DataPoint* p : candidates) {
        const double timeSec = FpSeconds(p->timestamp).count();

        // 화면에 보이는 모든 시리즈에 대해 거리 계산
        for(const auto& info : m_seriesInfoList) {
            if(info.isVisible) {
                const double yValue = info.extractor(QVariant::fromValue(p));
                // 현재 시리즈의 스크린 좌표를 계산
                QPointF screenPos = m_chart->mapToPosition(QPointF(timeSec, yValue), info.series);


                // 마우스 커서와 두 포인트 사이의 거리를 각각 계산
                double distance = QLineF(mouseScreenPos, screenPos).length();
                if(distance < minDistance) {
                    minDistance = distance;
                    nearestPoint = p;
                }
            }
        }
    }

    // 임계값 이내면 선택
    if(nearestPoint && minDistance <= config::View::Interaction::Proximity::Threshold) {
        emit pointHovered(*nearestPoint);
    }
}

void GraphWindow::onWaveformVisibilityChanged(int type, bool isVisible)
{
    // type 인덱스가 유효한 범위 내에 있는지 확인
    if(type >= 0 && type < m_seriesInfoList.size()) {
        m_seriesInfoList[type].isVisible = isVisible;
        m_seriesInfoList[type].series->setVisible(isVisible);

        emit redrawNeeded();
    }
}
// -----------------------

// ---- private -----
void GraphWindow::updateVisiblePoints(const std::deque<DataPoint>& data)
{
    // 축에서 초단위 시간 범위를 가져옴
    auto [minX_sec, maxX_sec] = getVisibleXRange(data);

    // std::chrono를 사용하여 초에서 나노초로 변환
    const auto minX_ns = std::chrono::duration_cast<Nanoseconds>(FpSeconds(minX_sec));
    const auto maxX_ns = std::chrono::duration_cast<Nanoseconds>(FpSeconds(maxX_sec));

    //보이는 범위의 반복자를 얻음
    auto [first, last] = getVisibleRangeIterators(data, minX_ns, maxX_ns);

    const int pointCount = std::distance(first, last);
    const int threshold = m_chartView->width(); // 픽셀 너비만큼 점을 뽑음

    if(pointCount > threshold) {
        std::vector<std::function<double(const DataPoint&)>> extractors(6);
        m_visibleDataPoints = downsampleLTTB(first, last, threshold, extractors);
    } else {
        m_visibleDataPoints.assign(first, last);
    }
}

void GraphWindow::updateSeriesData()
{
    for(auto& info : m_seriesInfoList) {
        info.points.clear();
        info.points.reserve(m_visibleDataPoints.size());
    }

    // 멤버 변수들을 채움
    for(const auto& p : std::as_const(m_visibleDataPoints)) {
        const double timeSec = FpSeconds(p.timestamp).count();
        for(auto& info : m_seriesInfoList) {
            info.points.emplace_back(timeSec, info.extractor(QVariant::fromValue(p)));
        }
    }

    // 멤버 변수들로 시리즈 업데이트
    for(const auto& info : m_seriesInfoList) {
        info.series->replace(info.points);
    }
}

void GraphWindow::updateAxes(const std::deque<DataPoint> &data)
{
    if(m_isAutoScrollEnabled) {
        if(m_visibleDataPoints.empty()) return;

        // Y값의 최소/최대값을 저장할 변수 초기화
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        // m_seriesInfoList 순회
        for(const auto& info : m_seriesInfoList) {
            // 현재 화면에 보이는 시리즈에 대해서만 min/max 계산
            if(info.isVisible) {
                // 해당 시리즈의 데이터를 순회
                for(const auto& point : info.points) {
                    minY = std::min(minY, point.y());
                    maxY = std::max(maxY, point.y());
                }
            }
        }

        // 유효한 경우에만 Y축 범위 설정
        if(minY <= maxY)
        // qDebug() << "minY: " << minY << " , maxY: " << maxY;
            updateYAxisRange(minY, maxY);

        // X축의 범위를 업데이트
        const auto [minX, maxX] = getVisibleXRange(data);
        m_axisX->setRange(minX, maxX);
    }
}

void GraphWindow::updateYAxisRange(double minY, double maxY)
{
    // 위아래 여백 계산
    double padding = (maxY - minY) * config::View::Padding::Ratio;
    if(padding < config::View::Padding::Min) {
        padding = config::View::Padding::Min; // 최소 여백 보장
    }

    m_axisY->setRange(minY - padding, maxY + padding);
    // qDebug() << "Range: (" << minY- padding << ", " << maxY + padding << ")";
}
// -------------------------------
