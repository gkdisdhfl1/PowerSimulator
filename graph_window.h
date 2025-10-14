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
    void pointHovered(const DataPoint& point);
    void redrawNeeded();
    void framePainted();

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void stretchGraph(double factor);
    void findNearestPoint(const QPointF& chartPos);
    void onWaveformVisibilityChanged(int type, bool isVisible);

private:
    struct SeriesInfo {
        QLineSeries* series = nullptr;
        std::function<double(const DataPoint&)> extractor;
        bool isVisible = false;
        QList<QPointF> points;
    };
    std::vector<SeriesInfo> m_seriesInfoList;

    // BaseGraphWindow에서 상속받음
    void setupSeries() override;

    // Y축 범위 계산 관련 함수들
    void updateYAxisRange(double minY, double maxY);

    // 데이터 처리 관련 함수들
    void updateVisiblePoints(const std::deque<DataPoint>& data);
    void updateSeriesData();
    void updateAxes(const std::deque<DataPoint>& data);

    // 차트 관련 객체 소유
    QValueAxis *m_axisY;

    std::vector<DataPoint> m_visibleDataPoints;
};

#endif // GRAPH_WINDOW_H
