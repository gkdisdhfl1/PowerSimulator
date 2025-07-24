#include "custom_chart_view.h"
#include <QtCharts/QChart>
#include <QMouseEvent>

CustomChartView::CustomChartView(QChart *chart, QWidget *parent) : QChartView(chart, parent)
    , m_isPanning(false)
{

}

void CustomChartView::mousePressEvent(QMouseEvent *event)
{
    emit userInteracted(); // 어떤 마우스 버튼이든 누르면 사용자 조작으로 간주

    if(event->button() == Qt::LeftButton) {
        // 왼쪽 버튼: 패닝 시작
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QChartView::mousePressEvent(event);
    }
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event)
{
    if(m_isPanning) {
        // 패닝 로직
        QPoint delta = event->pos() - m_panStartPos;
        chart()->scroll(-delta.x(), delta.y());
        m_panStartPos = event->pos();
        event->accept();
    } else {
        // 패닝 중이 아닐 때는 부모 클래스의 동작을 따름
        QChartView::mouseMoveEvent(event);
    }
}

void CustomChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_isPanning) {
        // 패닝 종료
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QChartView::mouseReleaseEvent(event);
    }
}

void CustomChartView::wheelEvent(QWheelEvent *event)
{
    emit userInteracted(); // 휠 조작도 사용자 조작

    // 휠 각도에 따라 줌 계산
    const double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    chart()->zoom(factor);

    event->accept();
}

void CustomChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 더블 클릭 시 줌 리셋
    chart()->zoomReset();
    event->accept();
}
