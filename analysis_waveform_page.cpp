#include "analysis_waveform_page.h"
#include "custom_chart_view.h"
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>

AnalysisWaveformPage::AnalysisWaveformPage(QWidget* parent)
    : BaseGraphWindow(nullptr, parent)
{
    setupSeries();
}

void AnalysisWaveformPage::setupSeries()
{
    m_chartView->setInteractive(false); // 스크롤, 줌 금지
    m_chart->legend()->hide(); // 범례 숨기기
    toggleAutoScroll(false); // 자동 스크롤 끄기

    m_axisY = new QValueAxis(this);
    m_axisY->setTitleText("V/A");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    // A상 전압/전류 시리즈
    m_voltageSeries = new QLineSeries(this);
    m_currentSeries = new QLineSeries(this);
    m_voltageSeries->setColor(QColor("blue"));
    m_currentSeries->setColor(QColor("red"));

    m_chart->addSeries(m_voltageSeries);
    m_chart->addSeries(m_currentSeries);
    m_voltageSeries->attachAxis(m_axisX);
    m_voltageSeries->attachAxis(m_axisY);
    m_currentSeries->attachAxis(m_axisX);
    m_currentSeries->attachAxis(m_axisY);
};

void AnalysisWaveformPage::updateWaveformData(const OneSecondSummaryData& data)
{
    const auto& waveData = data.lastTwoCycleData;
    if(waveData.empty()) return;

    // 데이터의 마지막 N개(2 사이클 분량)를 가져옴
    QList<QPointF> vPoints, iPoints;
    vPoints.reserve(waveData.size());
    iPoints.reserve(waveData.size());

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto& point : waveData) {
        double timeSec = std::chrono::duration<double>(point.timestamp).count();
        double v = point.voltage.a;
        double i = point.current.a;

        vPoints.append(QPointF(timeSec, v));
        iPoints.append(QPointF(timeSec, i));

        minY = std::min({minY, v, i});
        maxY = std::max({maxY, v, i});
    }

    // 시리즈 및 축 업데이트
    m_voltageSeries->replace(vPoints);
    m_currentSeries->replace(iPoints);

    if (!vPoints.isEmpty()) {
        m_axisX->setRange(vPoints.first().x(), vPoints.last().x());
        double padding = (maxY - minY) * 0.1 + 1.0;
        m_axisY->setRange(minY - padding, maxY + padding);
    }
}
