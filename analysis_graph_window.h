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
    explicit AnalysisGraphWindow(SimulationEngine* engine, QWidget *parent = nullptr);

signals:
    void autoScrollToggled(bool enabled); // 사용자가 그래프를 조작했을 때 ControlPanel에 알림
    void redrawNeeded(); // 재요청

public slots:
    void updateGraph(const std::deque<MeasuredData>& data);

private:
    void setupSeries() override;
    void updateAxes(const std::deque<MeasuredData>& data);
    void updateVisiblePoints(const std::deque<MeasuredData>& data);
    void updateSeriesData();
    void updateYAxisRange(double minY, double maxY);


    // 3개 Y축
    QValueAxis* m_axisY_voltage;
    QValueAxis* m_axisY_current;
    QValueAxis* m_axisY_power;

    std::vector<MeasuredData> m_visibleMeasuredData;
};

#endif // ANALYSIS_GRAPH_WINDOW_H
