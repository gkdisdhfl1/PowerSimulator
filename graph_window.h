#ifndef GRAPH_WINDOW_H
#define GRAPH_WINDOW_H

#include <deque>
#include "data_point.h"
#include "base_graph_window.h"

class QLineSeries;
class QValueAxis;

class GraphWindow : public BaseGraphWindow
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
    void framePainted();

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void stretchGraph(double factor);
    void findNearestPoint(const QPointF& chartPos);


private:    
    // BaseGraphWindow에서 상속받음
    void setupSeries() override;

    // Y축 범위 계산 관련 함수들
    void updateYAxisRange(double minY, double maxY);
    void updateMinMaxY(const QList<QPointF>& points, double& minY, double& maxY);

    // 데이터 처리 관련 함수들
    void updateVisiblePoints(const std::deque<DataPoint>& data);
    void updateSeriesData();
    void updateAxes(const std::deque<DataPoint>& data);


    // 차트 관련 객체 소유
    QLineSeries *m_voltageSeries;
    QLineSeries *m_currentSeries;
    QValueAxis *m_axisY;

    QList<QPointF> m_voltagePoints; // 현재 보이는 전압 데이터
    QList<QPointF> m_currentPoints; // 현재 보이는 전류 데이터
};

#endif // GRAPH_WINDOW_H
