#ifndef GRAPH_WINDOW_H
#define GRAPH_WINDOW_H

#include <QWidget>
// #include <QDialog>
#include <deque>
#include "data_point.h"

QT_BEGIN_NAMESPACE
class QChart;
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

public slots:
    void updateGraph(const std::deque<DataPoint>& data);
    void setGraphWidth(double width);

private:
    Ui::GraphWindow *ui;
    void setupChart(); // 차트 초기 설정을 위한 함수

    // 차트 관련 객체 소유
    QChart *m_chart;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QChartView * m_chartView;

    // 그래프 폭 조절
    double m_graphWidthSec;
};

#endif // GRAPH_WINDOW_H
