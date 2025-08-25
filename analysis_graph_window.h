#ifndef ANALYSIS_GRAPH_WINDOW_H
#define ANALYSIS_GRAPH_WINDOW_H

#include "base_graph_window.h"
#include "measured_data.h"
#include <deque>

class QLineSeries;

class AnalysisGraphWindow : public BaseGraphWindow
{
    Q_OBJECT
public:
    explicit AnalysisGraphWindow(QWidget *parent = nullptr);

public slots:
    void updateGraph(const std::deque<MeasuredData>& data);

private:
    void setupSeries() override;
    void updateAxes(const std::deque<MeasuredData>& data);

    // 3개 데이터 시리즈
    QLineSeries* m_voltageRmsSeries;
    QLineSeries* m_currentRmsSeries;
    QLineSeries* m_activePowerSeries;

    // 3개 Y축
    QValueAxis* m_axisY_voltage;
    QValueAxis* m_axisY_current;
    QValueAxis* m_axisY_power;
};

#endif // ANALYSIS_GRAPH_WINDOW_H
