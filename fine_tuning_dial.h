#ifndef FINE_TUNING_DIAL_H
#define FINE_TUNING_DIAL_H

#include <QDial>

class QMouseEvent;

class FineTuningDial : public QDial
{
    Q_OBJECT

public:
    explicit FineTuningDial(QWidget *parent = nullptr);

protected:
    // QWidget의 가상 함수를 재정의
    void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
    void doubleClicked();
};

#endif // FINE_TUNING_DIAL_H
