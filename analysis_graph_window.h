#ifndef ANALYSIS_GRAPH_WINDOW_H
#define ANALYSIS_GRAPH_WINDOW_H

#include "base_graph_window.h"
#include "measured_data.h"
#include <deque>

class QLineSeries;
class SimulationEngine;

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
    void toggleAutoScroll(bool enabled);

private:
    void setupSeries() override;
    void updateAxes(const std::deque<MeasuredData>& data);
    void updateVisiblePoints(const std::deque<MeasuredData>& data);
    void updateSeriesData();

    // 3개 데이터 시리즈
    QLineSeries* m_voltageRmsSeries;
    QLineSeries* m_currentRmsSeries;
    QLineSeries* m_activePowerSeries;

    // 3개 Y축
    QValueAxis* m_axisY_voltage;
    QValueAxis* m_axisY_current;
    QValueAxis* m_axisY_power;

    // 상태관리를 위한 멤버 변수
    SimulationEngine *m_engine;
    bool m_isAutoScrollEnabled;

    // 표시할 데이터 를 담을 멤버 변수
    QList<QPointF> m_voltagePoints;
    QList<QPointF> m_currentPoints;
    QList<QPointF> m_powerPoints;
};

#endif // ANALYSIS_GRAPH_WINDOW_H
