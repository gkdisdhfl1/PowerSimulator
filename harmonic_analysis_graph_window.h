#ifndef HARMONIC_ANALYSIS_GRAPH_WINDOW_H
#define HARMONIC_ANALYSIS_GRAPH_WINDOW_H

#include "base_graph_window.h"
#include "measured_data.h"

class QLineSeries;

class HarmonicAnalysisGraphWindow : public BaseGraphWindow
{
    Q_OBJECT
public:
    explicit HarmonicAnalysisGraphWindow(SimulationEngine *engine, QWidget *parent = nullptr);

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

    // 3개 데이터 시리즈
    QLineSeries* m_voltageRmsSeries;
    QLineSeries* m_currentRmsSeries;
    QLineSeries* m_activePowerSeries;

    // 3개 Y축
    QValueAxis* m_axisY_voltage;
    QValueAxis* m_axisY_current;
    QValueAxis* m_axisY_power;

    // 표시할 데이터 를 담을 멤버 변수
    QList<QPointF> m_voltagePoints;
    QList<QPointF> m_currentPoints;
    QList<QPointF> m_powerPoints;
};

#endif // HARMONIC_ANALYSIS_GRAPH_WINDOW_H
