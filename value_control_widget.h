#ifndef VALUE_CONTROL_WIDGET_H
#define VALUE_CONTROL_WIDGET_H

#include <QWidget>

namespace Ui {
class ValueControlWidget;
}

class ValueControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ValueControlWidget(QWidget *parent = nullptr);
    ~ValueControlWidget();

    void setRange(double min, double max);
    void setValue(double value);
    double value() const; // 현재 값을 가져오는 함수

protected:
    // QWidget의 함수를 재정의하여 더블클릭 감지
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(double value);

signals:
    void valueChanged(double newValue);

private:
    Ui::ValueControlWidget *ui;

    // 스케일 조정을 위한 변수
    double m_multiplier = 100.0;
};

#endif // VALUE_CONTROL_WIDGET_H
