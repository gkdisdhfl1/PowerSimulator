#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

// #include <QWidget>
#include <QDialog>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>
#include <deque>
#include "datapoint.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class GraphWindow;
}
QT_END_NAMESPACE

class GraphWindow : public QDialog
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
    QChart m_chart;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QChartView * m_chartView;
    double m_graphWidthSec;
};

#endif // GRAPHWINDOW_H
