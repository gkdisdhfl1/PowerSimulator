#ifndef CUSTOM_CHART_VIEW_H
#define CUSTOM_CHART_VIEW_H

#include <QtCharts/QChartView>
#include <QPointF>

class CustomChartView : public QChartView
{
    Q_OBJECT

public:
    explicit CustomChartView(QChart *chart, QWidget *parent = nullptr);

signals:
    // 사용자가 그래프와 상호작용했음을 외부에 알리는 시그널
    void userInteracted();
    void stretchRequested(double factor);
    void mouseMoved(const QPointF& point);

protected:
    // 재정의할 마우스 및 휠 이벤트 헨들러
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    bool m_isPanning;
    bool m_ctrlIsPressed;
    QPoint m_panStartPos;
};

#endif // CUSTOM_CHART_VIEW_H
