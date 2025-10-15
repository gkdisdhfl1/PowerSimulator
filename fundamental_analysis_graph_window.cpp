#include "custom_chart_view.h"
#include "simulation_engine.h"
#include "fundamental_analysis_graph_window.h"
#include "analysis_utils.h"
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>



FundamentalAnalysisGraphWindow::FundamentalAnalysisGraphWindow(SimulationEngine *engine, QWidget *parent)
    : BaseGraphWindow(engine, parent)
    , m_voltageRmsSeries(new QLineSeries(this))
    , m_currentRmsSeries(new QLineSeries(this))
    , m_activePowerSeries(new QLineSeries(this))
    , m_axisY_voltage(new QValueAxis(this))
    , m_axisY_current(new QValueAxis(this))
    , m_axisY_power(new QValueAxis(this))
{
    m_chart->setTitle("Fundamental Component Analysis");

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

void FundamentalAnalysisGraphWindow::setupSeries()
{
    // 시리즈 생성 및 이름/색상 설정
    m_voltageRmsSeries->setName("Voltage RMS (Fund.)");
    m_currentRmsSeries->setName("Current RMS (Fund.)");
    m_activePowerSeries->setName("Active Power (Fund.)");

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

void FundamentalAnalysisGraphWindow::updateGraph(const std::deque<MeasuredData>& data)
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

void FundamentalAnalysisGraphWindow::updateVisiblePoints(const std::deque<MeasuredData>& data)
{
    // 축에서 초단위 시간 범위를 가져옴
    auto [minX_sec, maxX_sec] = getVisibleXRange(data);

    // std::chrono를 사용하여 초에서 나노초로 변환
    const auto minX_ns = std::chrono::duration_cast<Nanoseconds>(FpSeconds(minX_sec));
    const auto maxX_ns = std::chrono::duration_cast<Nanoseconds>(FpSeconds(maxX_sec));

    //보이는 범위의 반복자를 얻음
    auto [first, last] = getVisibleRangeIterators(data, minX_ns, maxX_ns);

    // LTTB 다운샘플링을 위한 데이터 추출기 정의
    std::vector<std::function<double(const MeasuredData&)>> extractors {
        [](const MeasuredData& d) {
            const auto& v = d.fundamentalVoltage;
            return (v.order > 0) ? v.rms : 0.0;
        },
        [](const MeasuredData& d) {
            const auto& i = d.fundamentalCurrent;
            return (i.order > 0) ? i.rms : 0.0;
        },
        [](const MeasuredData& d) {
            const auto& v = d.fundamentalVoltage;
            const auto& i = d.fundamentalCurrent;
            return AnalysisUtils::calculateActivePower(&v, &i);
        }

    };

    const int pointCount = std::distance(first, last);
    const int threshold = m_chartView->width();

    m_voltagePoints.clear();
    m_currentPoints.clear();
    m_powerPoints.clear();

    if(pointCount > threshold && threshold > 0) {
        auto sample_data = downsampleLTTB(first, last, threshold, extractors);
        for(const auto& d : sample_data) {
            const double timeSec = FpSeconds(d.timestamp).count();
            const auto& v_fund = d.fundamentalVoltage;
            const auto& i_fund = d.fundamentalCurrent;

            m_voltagePoints.append(QPointF(timeSec, v_fund.order > 0 ? v_fund.rms : 0.0));
            m_currentPoints.append(QPointF(timeSec, i_fund.order > 0 ? i_fund.rms : 0.0));
            m_powerPoints.append(QPointF(timeSec, AnalysisUtils::calculateActivePower(&v_fund, &i_fund)));
        }
    } else {
        // 다운샘플링 안할 때도 동일한 로직으로 데이터 추출
        for(auto it = first; it != last; ++it) {
            const double timeSec = FpSeconds(it->timestamp).count();
            const auto& v_fund = it->fundamentalVoltage;
            const auto& i_fund = it->fundamentalCurrent;

            m_voltagePoints.append(QPointF(timeSec, v_fund.order > 0 ? v_fund.rms : 0.0));
            m_currentPoints.append(QPointF(timeSec, i_fund.order > 0 ? i_fund.rms : 0.0));
            m_powerPoints.append(QPointF(timeSec, AnalysisUtils::calculateActivePower(&v_fund, &i_fund)));
        }
    }
}

void FundamentalAnalysisGraphWindow::updateSeriesData()
{
    m_voltageRmsSeries->replace(m_voltagePoints);
    m_currentRmsSeries->replace(m_currentPoints);
    m_activePowerSeries->replace(m_powerPoints);
}

void FundamentalAnalysisGraphWindow::updateAxes(const std::deque<MeasuredData>& data)
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
