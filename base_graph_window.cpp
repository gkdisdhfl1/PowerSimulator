#include "base_graph_window.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"
#include "data_point.h"
#include <QChart>
#include <QValueAxis>
#include <QGridLayout>

template std::pair<double, double> BaseGraphWindow::getVisibleXRange<std::deque<DataPoint>>(const std::deque<DataPoint>&);
template std::pair<double, double> BaseGraphWindow::getVisibleXRange<std::deque<MeasuredData>>(const std::deque<MeasuredData>&);



BaseGraphWindow::BaseGraphWindow(SimulationEngine *engine, QWidget *parent)
    :QWidget(parent)
    , m_chart(std::make_unique<QChart>())
    , m_axisX(new QValueAxis())
    , m_chartView(new CustomChartView(m_chart.get(), this))
    , m_engine(engine)
    , m_isAutoScrollEnabled(true)
{
    setupBaseChart();
}

void BaseGraphWindow::setupBaseChart()
{
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 레이아웃 설정
    auto mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_chartView);

    // 차트 기본 설정
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    // X축 설정
    m_axisX->setLabelFormat(tr("%.1f s"));
    m_axisX->setTitleText(tr("시간 (s)"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
}

template<typename Container>
std::pair<double, double> BaseGraphWindow::getVisibleXRange(const Container& data)
{
    double minX, maxX;

    if(m_isAutoScrollEnabled) {
        if(data.empty()) {
            return {0.0, m_engine->parameters().graphWidthSec};
        }

        // C++20 concept를 사용하여 일반화 가능
        // 여기서는 간단하게 각 데이터 타입 확인
        double lastTimestamp;
        if constexpr (std::is_same_v<typename Container::value_type, DataPoint>) {
            lastTimestamp = std::chrono::duration<double>(data.back().timestamp).count();
        } else { // MeasuredData
            lastTimestamp = std::chrono::duration<double>(data.back().timestamp).count();
        }

        const double graphWidth = m_engine->parameters().graphWidthSec;
        minX = (lastTimestamp < graphWidth) ? 0 : lastTimestamp - graphWidth;
        maxX = (lastTimestamp < graphWidth) ? graphWidth : lastTimestamp;
    } else {
        // 자동 스크롤 모드가 아닐 때는 현재 X축 범위를 그대로 사용
        minX = m_axisX->min();
        maxX = m_axisX->max();
    }
    return {minX, maxX};
}

void BaseGraphWindow::toggleAutoScroll(bool enabled)
{
    m_isAutoScrollEnabled = enabled;
}
