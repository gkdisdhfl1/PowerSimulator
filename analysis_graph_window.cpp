#include "analysis_graph_window.h"
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QDebug>
#include <QPen>

AnalysisGraphWindow::AnalysisGraphWindow(QWidget *parent)
    : BaseGraphWindow(parent)
    , m_voltageRmsSeries(new QLineSeries(this))
    , m_currentRmsSeries(new QLineSeries(this))
    , m_activePowerSeries(new QLineSeries(this))
    , m_axisY_voltage(new QValueAxis(this))
    , m_axisY_current(new QValueAxis(this))
    , m_axisY_power(new QValueAxis(this))
{
    m_chart->setTitle("Cycle-based Analysis");
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

    // y축 생성 및 설정
    // 전압 왼쪽
    m_axisY_voltage->setTitleText("Voltage (V)");
    m_axisY_voltage->setLinePenColor(m_voltageRmsSeries->color());
    m_axisY_voltage->setLabelFormat("%.1f");
    m_chart->addAxis(m_axisY_voltage, Qt::AlignLeft);
    m_voltageRmsSeries->attachAxis(m_axisX);
    m_voltageRmsSeries->attachAxis(m_axisY_voltage);

    // 전류 오른쪽
    m_axisY_current->setTitleText("Current (V)");
    m_axisY_current->setLinePenColor(m_currentRmsSeries->color());
    m_axisY_current->setLabelFormat("%.2f");
    m_chart->addAxis(m_axisY_current, Qt::AlignRight);
    m_currentRmsSeries->attachAxis(m_axisX);
    m_currentRmsSeries->attachAxis(m_axisY_current);

    // 전력 오른쪽
    m_axisY_power->setTitleText("Power (W)");
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

    QList<QPointF> voltagePoints, currentPoints, powerPoints;
    voltagePoints.reserve(data.size());
    currentPoints.reserve(data.size());
    powerPoints.reserve(data.size());

    for(const auto& d : data) {
        const double timeSec = std::chrono::duration<double>(d.timestamp).count();
        voltagePoints.append(QPointF(timeSec, d.voltageRms));
        currentPoints.append(QPointF(timeSec, d.currentRms));
        powerPoints.append(QPointF(timeSec, d.activePower));
    }

    m_voltageRmsSeries->replace(voltagePoints);
    m_currentRmsSeries->replace(currentPoints);
    m_activePowerSeries->replace(powerPoints);

    updateAxes(data);
}

void AnalysisGraphWindow::updateAxes(const std::deque<MeasuredData>& data)
{
    if(data.empty()) return;

    // x축 범위 설정
    const double minX = std::chrono::duration<double>(data.front().timestamp).count();
    const double maxX = std::chrono::duration<double>(data.back().timestamp).count();
    m_axisX->setRange(minX, maxX > minX ? maxX : minX + 1);

    // 각 y축의 범위를 독립적으로 계산
    auto calculateRange = [](const auto& series, auto& axis) {
        if(series->points().isEmpty()) return;
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();
        for(const auto& p : series->points()) {
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        double padding = (maxY - minY) * 0.1;
        padding = std::max(padding, 0.5);
        axis->setRange(minY - padding, maxY + padding);
    };

    calculateRange(m_voltageRmsSeries, m_axisY_voltage);
    calculateRange(m_currentRmsSeries, m_axisY_current);
    calculateRange(m_activePowerSeries, m_axisY_power);
}
