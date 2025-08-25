#ifndef BASE_GRAPH_WINDOW_H
#define BASE_GRAPH_WINDOW_H

#include <QWidget>

// 전방 선언
class QChart;
class QValueAxis;
class CustomChartView;

class BaseGraphWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BaseGraphWindow(QWidget *parent = nullptr);
    virtual ~BaseGraphWindow() = default;

protected:
    void setupBaseChart();
    virtual void setupSeries() = 0; // 순수 가상 함수

    // 멤버 변수들을 protected로 이동하여 자식 클래스에서 접근 가능하도록 함
    std::unique_ptr<QChart> m_chart;
    QValueAxis *m_axisX;
    CustomChartView *m_chartView;
};

#endif // BASE_GRAPH_WINDOW_H
