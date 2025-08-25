#ifndef GRAPH_WINDOW_H
#define GRAPH_WINDOW_H

#include <QWidget>
#include <deque>
#include "data_point.h"
#include "custom_chart_view.h"

QT_BEGIN_NAMESPACE
class QLineSeries;
class QValueAxis;
class QChart;


QT_END_NAMESPACE
class SimulationEngine;

class GraphWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWindow(SimulationEngine* engine, QWidget *parent = nullptr);
    ~GraphWindow();

signals:
    // 자동 스크롤 상태가 변경되었음을 알리는 시그널
    void autoScrollToggled(bool enabled);
    void pointHovered(const QPointF& point);
    void redrawNeeded();

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void toggleAutoScroll(bool enabled); // 자동 스크롤 토글 슬롯
    void stretchGraph(double factor);
    void findNearestPoint(const QPointF& chartPos);


private:
    SimulationEngine *m_engine;

    void setupChart(); // 차트 초기 설정을 위한 함수
    void updateYAxisRange(double minY, double maxY);
    void updateMinMaxY(const QList<QPointF>& points, double& minY, double& maxY);
    void updateVisiblePoints(const std::deque<DataPoint>& data);
    void updateSeriesData();
    void updateAxesRanges();

    // 차트 관련 객체 소유
    std::unique_ptr<QChart> m_chart;
    QLineSeries *m_series;
    QLineSeries *m_currentSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    CustomChartView *m_chartView;
    QList<QPointF> m_voltagePoints; // 현재 보이는 전압 데이터
    QList<QPointF> m_currentPoints; // 현재 보이는 전류 데이터

    // 그래프 폭 조절
    bool m_isAutoScrollEnabled;
};

#endif // GRAPH_WINDOW_H
