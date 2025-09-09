#include "analysis_graph_window.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QDebug>
#include <QPen>
#include <ranges>

using utils::FpSeconds;
using utils::Nanoseconds;

AnalysisGraphWindow::AnalysisGraphWindow(SimulationEngine *engine, QWidget *parent)
    : BaseGraphWindow(engine, parent)
    , m_voltageRmsSeries(new QLineSeries(this))
    , m_currentRmsSeries(new QLineSeries(this))
    , m_activePowerSeries(new QLineSeries(this))
    , m_axisY_voltage(new QValueAxis(this))
    , m_axisY_current(new QValueAxis(this))
    , m_axisY_power(new QValueAxis(this))
{
    m_chart->setTitle("Cycle-based Analysis");

    // 사용자가 그래프 조작 시 자동  스크롤 해제 및 ControlPanel에 알림
    connect(m_chartView, &CustomChartView::userInteracted, this, [this]() {
        if(m_isAutoScrollEnabled) {
            toggleAutoScroll(false);
            emit autoScrollToggled(false);
        }
        emit redrawNeeded();
    });

    // 더블 클릭 시 줌 리셋 및 전체 보기를 위한 데이터 재요청
    connect(m_chartView, &CustomChartView::doubleClicked, this, [this]() {
        m_chart->zoomReset();
        if(!m_isAutoScrollEnabled) {
            emit redrawNeeded();
        }
    });

    m_voltageRmsSeries->setPointsVisible(true);
    m_currentRmsSeries->setPointsVisible(true);
    m_activePowerSeries->setPointsVisible(true);

    setupSeries();
}

void AnalysisGraphWindow::setupSeries()
{
    // 시리즈 생성 및 이름/색상 설정
    m_voltageRmsSeries->setName("Voltage RMS");
    m_currentRmsSeries->setName("Current RMS");
    m_activePowerSeries->setName("Active Power");

    m_voltageRmsSeries->setColor(QColor("blue"));
    m_currentRmsSeries->setColor(QColor("red"));
    m_activePowerSeries->setColor(QColor("green"));

    m_chart->addSeries(m_voltageRmsSeries);
    m_chart->addSeries(m_currentRmsSeries);
    m_chart->addSeries(m_activePowerSeries);

    m_axisX->setRange(0, m_engine->parameters().graphWidthSec ); // 초기 범위를 설정값으로


    // y축 생성 및 설정
    // 전압 왼쪽
    m_axisY_voltage->setTitleText("전압 (V)");
    m_axisY_voltage->setLinePenColor(m_voltageRmsSeries->color());
    m_axisY_voltage->setLabelFormat("%.1f");
    m_chart->addAxis(m_axisY_voltage, Qt::AlignLeft);
    m_voltageRmsSeries->attachAxis(m_axisX);
    m_voltageRmsSeries->attachAxis(m_axisY_voltage);

    // 전류 오른쪽
    m_axisY_current->setTitleText("전류 (V)");
    m_axisY_current->setLinePenColor(m_currentRmsSeries->color());
    m_axisY_current->setLabelFormat("%.2f");
    m_chart->addAxis(m_axisY_current, Qt::AlignRight);
    m_currentRmsSeries->attachAxis(m_axisX);
    m_currentRmsSeries->attachAxis(m_axisY_current);

    // 전력 오른쪽
    m_axisY_power->setTitleText("전력 (W)");
    m_axisY_power->setLinePenColor(m_activePowerSeries->color());
    m_axisY_power->setLabelFormat("%.0f");
    m_chart->addAxis(m_axisY_power, Qt::AlignRight);
    m_activePowerSeries->attachAxis(m_axisX);
    m_activePowerSeries->attachAxis(m_axisY_power);
}

void AnalysisGraphWindow::updateGraph(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        m_voltageRmsSeries->clear();
        m_currentRmsSeries->clear();
        m_activePowerSeries->clear();
        return;
    }

    updateVisiblePoints(data);
    updateSeriesData();
    updateAxes(data);
}

void AnalysisGraphWindow::updateVisiblePoints(const std::deque<MeasuredData>& data)
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
        // Vrms, Irms, Power 값을 추출하는 람다의 벡터를 전달
        std::vector<std::function<double(const MeasuredData&)>> extractors = {
            [](const MeasuredData& d) { return d.voltageRms; },
            [](const MeasuredData& d) { return d.currentRms; },
            [](const MeasuredData& d) { return d.activePower; },
        };

        auto sampled_data = downsampleLTTB(first, last, threshold, extractors);

        // LTTB 결과로 멤버 변수 채우기
        m_voltagePoints.clear();
        m_currentPoints.clear();
        m_powerPoints.clear();

        for(const auto& d : sampled_data) {
            const double timeSec = FpSeconds(d.timestamp).count();
            m_voltagePoints.append(QPointF(timeSec, d.voltageRms));
            m_currentPoints.append(QPointF(timeSec, d.currentRms));
            m_powerPoints.append(QPointF(timeSec, d.activePower));
        }
    } else {
        m_voltagePoints.clear();
        m_currentPoints.clear();
        m_powerPoints.clear();

        for(auto it = first; it != last; ++it) {
            const double timeSec = FpSeconds(it->timestamp).count();
            m_voltagePoints.append(QPointF(timeSec, it->voltageRms));
            m_currentPoints.append(QPointF(timeSec, it->currentRms));
            m_powerPoints.append(QPointF(timeSec, it->activePower));
        }
    }
}

void AnalysisGraphWindow::updateSeriesData()
{
    m_voltageRmsSeries->replace(m_voltagePoints);
    m_currentRmsSeries->replace(m_currentPoints);
    m_activePowerSeries->replace(m_powerPoints);
}

void AnalysisGraphWindow::updateAxes(const std::deque<MeasuredData>& data)
{
    auto calculateRange = [](const auto& points, auto& axis) {
        if(points.isEmpty()) return;
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();
        for(const auto& p : points) {
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        double padding = (maxY - minY) * 0.1;
        padding = std::max(padding, 0.5);
        axis->setRange(minY - padding, maxY + padding);
    };

    if(m_isAutoScrollEnabled) {
        calculateRange(m_voltagePoints, m_axisY_voltage);
        calculateRange(m_currentPoints, m_axisY_current);
        calculateRange(m_powerPoints, m_axisY_power);

        const auto [minX, maxX] = getVisibleXRange(data);
        m_axisX->setRange(minX, maxX);
    }
}
