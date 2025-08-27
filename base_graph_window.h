#ifndef BASE_GRAPH_WINDOW_H
#define BASE_GRAPH_WINDOW_H

#include <QWidget>

// 전방 선언
class QChart;
class QValueAxis;
class CustomChartView;
class SimulationEngine;

class BaseGraphWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BaseGraphWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    virtual ~BaseGraphWindow() = default;

public slots:
    void toggleAutoScroll(bool enabled);

protected:
    void setupBaseChart();
    virtual void setupSeries() = 0; // 순수 가상 함수

    // 멤버 변수들을 protected로 이동하여 자식 클래스에서 접근 가능하도록 함
    std::unique_ptr<QChart> m_chart;
    QValueAxis *m_axisX;
    CustomChartView *m_chartView;
    SimulationEngine* m_engine;
    bool m_isAutoScrollEnabled;

    // X축의 현재 보이는 범위를 계산하는 헬퍼 함수
    template<typename Container>
    std::pair<double, double> getVisibleXRange(const Container& data);
};

#endif // BASE_GRAPH_WINDOW_H
