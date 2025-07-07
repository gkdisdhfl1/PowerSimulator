#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

// #include <QWidget>
#include <QDialog>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>

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

public slots:
    void updateGraph(const QVector<QPointF>& data);

private:
    Ui::GraphWindow *ui;
    void setupChart(); // 차트 초기 설정을 위한 함수

    // 차트 관련 객체 소유
    QChart *m_chart;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QChartView * m_chartView;
};

#endif // GRAPHWINDOW_H
