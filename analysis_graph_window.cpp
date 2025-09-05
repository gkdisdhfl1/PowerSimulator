#include "analysis_graph_window.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QDebug>
#include <QPen>
#include <ranges>

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
    m_voltagePoints.clear();
    m_currentPoints.clear();
    m_powerPoints.clear();

    const auto [minX, maxX] = getVisibleXRange(data);

    auto pointsView = data
                      | std::ranges::views::transform([](const MeasuredData& d) {
                            const double timeSec = std::chrono::duration<double>(d.timestamp).count();
                            return std::make_tuple(
                                QPointF(timeSec, d.voltageRms),
                                QPointF(timeSec, d.currentRms),
                                QPointF(timeSec, d.activePower)
                                );
                        })
                      | std::views::filter([minX, maxX](const auto& tpl) {
                            // 튜플의 첫 번째 qPointF의 x좌표를 기준으로 필터링
                            return std::get<0>(tpl).x() >= minX && std::get<0>(tpl).x() <= maxX;
                        });

    // 필터링된 결과를 멤버 변수에 저장
    for(const auto& [voltageP, currentP, powerP]: pointsView) {
        m_voltagePoints.append(voltageP);
        m_currentPoints.append(currentP);
        m_powerPoints.append(powerP);
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

    calculateRange(m_voltagePoints, m_axisY_voltage);
    calculateRange(m_currentPoints, m_axisY_current);
    calculateRange(m_powerPoints, m_axisY_power);

    // X축 범위는 자동  스크롤 모드일 때만 업데이트
    if(m_isAutoScrollEnabled) {
        const auto [minX, maxX] = getVisibleXRange(data);
        m_axisX->setRange(minX, maxX);
    } else {
        // // 꺼져있을 때는 사용자가 줌/팬한 상태이므로 X축 범위를 변경하지 않음
        // if(!data.empty()) {
        //     const double minX = std::chrono::duration<double>(data.front().timestamp).count();
        //     const double maxX = std::chrono::duration<double>(data.back().timestamp).count();
        //     m_axisX->setRange(minX, maxX > minX ? maxX : minX + 1);
        // }
    }
}
