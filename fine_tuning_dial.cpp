#include "fine_tuning_dial.h"

FineTuningDial::FineTuningDial(QWidget *parent)
    : QDial(parent)
{}

// 마우스 더블클릭 이벤트 핸들러 구현
void FineTuningDial::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit doubleClicked();

    QDial::mouseDoubleClickEvent(event);
}
