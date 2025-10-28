#include "analysis_graph_window.h"
#include "custom_chart_view.h"
#include "simulation_engine.h"
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QDebug>
#include <QPen>

using utils::FpSeconds;
using utils::Nanoseconds;

AnalysisGraphWindow::AnalysisGraphWindow(SimulationEngine *engine, QWidget *parent)
    : BaseGraphWindow(engine, parent)
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

    setupSeries();
}

void AnalysisGraphWindow::setupSeries()
{
    // y축 생성 및 설정
    // 전압 왼쪽
    m_axisY_voltage->setTitleText("전압 (V)");
    m_axisY_voltage->setLabelFormat("%.1f");
    m_chart->addAxis(m_axisY_voltage, Qt::AlignLeft);

    // 전류 오른쪽
    m_axisY_current->setTitleText("전류 (V)");
    m_axisY_current->setLabelFormat("%.2f");
    m_chart->addAxis(m_axisY_current, Qt::AlignRight);

    // 전력 오른쪽
    m_axisY_power->setTitleText("전력 (W)");
    m_axisY_power->setLabelFormat("%.0f");
    m_chart->addAxis(m_axisY_power, Qt::AlignRight);

    // --- m_seriesInfoList 초기화 ---
    // A상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().voltageRms.a;},
        m_axisY_voltage,
        true, {}
    });
    m_seriesInfoList.back().series->setName("Voltage RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::blue));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().currentRms.a;},
        m_axisY_current,
        true, {}
    });
    m_seriesInfoList.back().series->setName("Current RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::red));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().activePower.a;},
        m_axisY_power,
        true, {}
    });
    m_seriesInfoList.back().series->setName("Active Power");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::green));

    // B상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().voltageRms.b;},
        m_axisY_voltage,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Voltage B RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::yellow));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().currentRms.b;},
        m_axisY_current,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Current B RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::orange));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().activePower.b;},
        m_axisY_power,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Active Power B");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::limegreen));

    // C상
    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().voltageRms.c;},
        m_axisY_voltage,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Voltage C RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::black));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().currentRms.c;},
        m_axisY_current,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Current C RMS");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::gray));

    m_seriesInfoList.emplace_back(SeriesInfo{
        new QLineSeries(this),
        [](const QVariant& v) { return v.value<MeasuredData>().activePower.c;},
        m_axisY_power,
        false, {}
    });
    m_seriesInfoList.back().series->setName("Active Power C");
    m_seriesInfoList.back().series->setColor(QColor(QColorConstants::Svg::lightgreen));
    // -------------------------

    // X축 설정
    m_axisX->setRange(0, m_engine->m_graphWidthSec.value() ); // 초기 범위를 설정값으로
    for(const auto& info : m_seriesInfoList) {
        m_chart->addSeries(info.series);
        info.series->attachAxis(m_axisX);
        info.series->attachAxis(info.yAxis);
        info.series->setVisible(info.isVisible);
        info.series->setPointsVisible(true);
    }
}

void AnalysisGraphWindow::updateGraph(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        for(const auto& info : m_seriesInfoList) {
            info.series->clear();
        }
        return;
    }

    updateVisiblePoints(data);
    updateSeriesData();
    updateAxes(data);
}

void AnalysisGraphWindow::onWaveformVisibilityChanged(int type, bool isVisible)
{
    // type 인덱스가 유효한 범위 내에 있는지 확인
    if(type >= 0 && type < m_seriesInfoList.size()) {
        // qDebug() << "onWaveformVisibilityChanged: type = " << type << ", isVisible = " << isVisible;
        m_seriesInfoList[type].isVisible = isVisible;
        m_seriesInfoList[type].series->setVisible(isVisible);

        emit redrawNeeded();
    }
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
        std::vector<std::function<double(const MeasuredData&)>> extractors;
        for(const auto& info : m_seriesInfoList) {
            if(info.isVisible) {
                extractors.push_back([&info](const MeasuredData& d) {
                    return info.extractor(QVariant::fromValue(d));
                });
            }
        }
        if(extractors.empty()) {
            m_visibleMeasuredData.assign(first, last);
        } else {
            m_visibleMeasuredData = downsampleLTTB(first, last, threshold, extractors);
        }
    } else {
        m_visibleMeasuredData.assign(first, last);
    }
}

void AnalysisGraphWindow::updateSeriesData()
{
    for(auto& info : m_seriesInfoList) {
        info.points.clear();
        info.points.reserve(m_visibleMeasuredData.size());
    }

    // 멤버 변수들을 채움
    for(const auto& d : std::as_const(m_visibleMeasuredData)) {
        const double timeSec = FpSeconds(d.timestamp).count();
        for(size_t i = 0; i < m_seriesInfoList.size(); ++i) {
            auto& info = m_seriesInfoList[i];
            double yValue = info.extractor(QVariant::fromValue(d));

            info.points.emplace_back(timeSec, yValue);
        }
    }

    // 멤버 변수들로 시리즈 업데이트
    for(const auto& info : m_seriesInfoList) {
        info.series->replace(info.points);
    }
}

void AnalysisGraphWindow::updateAxes(const std::deque<MeasuredData>& data)
{
    if(m_isAutoScrollEnabled) {
        double voltageMin = std::numeric_limits<double>::max();
        double voltageMax = std::numeric_limits<double>::lowest();
        double currentMin = std::numeric_limits<double>::max();
        double currentMax = std::numeric_limits<double>::lowest();
        double powerMin = std::numeric_limits<double>::max();
        double powerMax = std::numeric_limits<double>::lowest();

        // m_seriesInfoList를 직접 순회
        for(const auto& info : m_seriesInfoList) {
            if(info.isVisible && !info.points.isEmpty()) {
                // 이 시리즈가 어떤 축에 연결되어 있는지 확인
                if(info.series->attachedAxes().contains(m_axisY_voltage)) {
                    for(const auto& p : info.points) {
                        voltageMin = std::min(voltageMin, p.y());
                        voltageMax = std::max(voltageMax, p.y());
                    }
                } else if(info.series->attachedAxes().contains(m_axisY_current)) {
                    for(const auto& p : info.points) {
                        currentMin = std::min(currentMin, p.y());
                        currentMax = std::max(currentMax, p.y());
                    }
                } else if(info.series->attachedAxes().contains(m_axisY_power)) {
                    for(const auto& p : info.points) {
                        powerMin = std::min(powerMin, p.y());
                        powerMax = std::max(powerMax, p.y());
                    }
                }
            }
        }

        // 각 축의 범위설정
        auto setAxisRange = [](auto& axis, double minVal, double maxVal) {
            if(minVal <= maxVal) {
                double padding = (maxVal - minVal) * 0.1;
                padding = std::max(padding, 0.5);
                axis->setRange(minVal - padding, maxVal + padding);
            }
        };

        setAxisRange(m_axisY_voltage, voltageMin, voltageMax);
        setAxisRange(m_axisY_current, currentMin, currentMax);
        setAxisRange(m_axisY_power, powerMin, powerMax);

        // X축 업데이트
        const auto [minX, maxX] = getVisibleXRange(data);
        m_axisX->setRange(minX,maxX);
    }
}
