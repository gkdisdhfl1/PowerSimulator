#include "custom_chart_view.h"
#include "config.h"
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractSeries>

CustomChartView::CustomChartView(QChart *chart, QWidget *parent) : QChartView(chart, parent)
    , m_isPanning(false)
    , m_ctrlIsPressed(false)
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
    if(!chart()->series().isEmpty()) {
        // 화면 좌표를 차트의 값 좌표로 변환
        const QPointF chartPoint = chart()->mapToValue(event->pos(), chart()->series().first());
        emit mouseMoved(chartPoint);
    }

    if(m_isPanning) {
        // 패닝 로직
        QPoint delta = event->pos() - m_panStartPos;
        chart()->scroll(-delta.x(), delta.y());
        m_panStartPos = event->pos();
        emit userInteracted();
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
    const double factor = event->angleDelta().y() > 0 ?
                              config::View::Interaction::Zoom::FactorIn :
                              config::View::Interaction::Zoom::FactorOut;

    const Qt::KeyboardModifiers modifier = event->modifiers();

    if(modifier == Qt::ShiftModifier) {
        // shift + 휠: 스트레칭 줌
        emit stretchRequested(factor);
    } else if(modifier == Qt::ControlModifier) {
        // ctrl + 휠: 고정 줌(자동 스크롤 해제)
        emit userInteracted();

        // 차트 축 목록에서 첫 번째 수평 축을 찾음
        const auto& horizontalAxes = chart()->axes(Qt::Horizontal);
        QValueAxis *axisX = nullptr;
        for (auto axis : horizontalAxes) {
            axisX = qobject_cast<QValueAxis*>(axis);
            if(axisX) break; // 첫 번째 QValueAxis를 찾으면 중단
        }

        // nullptr 체크
        if(!axisX)
            return; // 유효한 X축이 없으면 아무것도 하지 않고 반환.

        if(chart()->series().isEmpty()) return;

        const double currentMin = axisX->min();
        const double currentMax = axisX->max();
        const double currentWidth = currentMax - currentMin;

        // 마우스 커서 위치를 기준으로 새로운 범위 계산
        const QPointF mousePos = event->position();
        // 첫 번째 시리즈를 기준으로 값을 변환
        const QPointF chartPos = chart()->mapToValue(mousePos, chart()->series().first());
        const double center = chartPos.x();

        const double newWidth = currentWidth / factor;
        const double newMin = center - (center - currentMin) / currentWidth * newWidth;
        const double newMax = center + (currentMax - center) / currentWidth * newWidth;

        axisX->setRange(newMin, newMax);
    } else {
        emit userInteracted();

        // 휠 각도에 따라 줌 계산
        chart()->zoom(factor);
    }
    event->accept();
}

void CustomChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 더블 클릭 시 줌 리셋
    chart()->zoomReset();
    event->accept();
}
