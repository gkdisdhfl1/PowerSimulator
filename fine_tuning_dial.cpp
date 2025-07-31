#include "fine_tuning_dial.h"
#include <QWheelEvent>

FineTuningDial::FineTuningDial(QWidget *parent)
    : QDial(parent)
{}

// 마우스 더블클릭 이벤트 핸들러 구현
void FineTuningDial::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit doubleClicked();

    QDial::mouseDoubleClickEvent(event);
}

void FineTuningDial::wheelEvent(QWheelEvent *event)
{
    // 휠이 움직인 방향과 각도를 가져옴
    const QPoint angleDelta = event->angleDelta();

    if(angleDelta.y() > 0) {
        // 위로 굴렸을 때
        if(value() == maximum())
            setValue(0);
        else
            setValue(value() + 1);
    } else if(angleDelta.y() < 0) {
        if(value() == minimum())
            setValue(359);
        else
            setValue(value() - 1);
    }

    event->accept();
}
