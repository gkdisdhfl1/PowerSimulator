#ifndef GRAPH_WINDOW_H
#define GRAPH_WINDOW_H

#include <QWidget>
// #include <QDialog>
#include <deque>
#include <memory>
#include "data_point.h"
#include "custom_chart_view.h"

QT_BEGIN_NAMESPACE
class QLineSeries;
class QValueAxis;
class QChartView;

namespace Ui {
class GraphWindow;
}
QT_END_NAMESPACE

class GraphWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    ~GraphWindow();
    double getGraphWidth() const;

signals:
    // 자동 스크롤 상태가 변경되었음을 알리는 시그널
    void autoScrollToggled(bool enabled);

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void setGraphWidth(double width);
    void toggleAutoScroll(bool enabled); // 자동 스크롤 토글 슬롯
    void stretchGraph(double factor);

private:
    Ui::GraphWindow *ui;
    void setupChart(); // 차트 초기 설정을 위한 함수

    // 차트 관련 객체 소유
    std::unique_ptr<QChart> m_chart;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    CustomChartView *m_chartView;

    // 그래프 폭 조절
    double m_graphWidthSec;

    bool m_isAutoScrollEnabled;
};

#endif // GRAPH_WINDOW_H
