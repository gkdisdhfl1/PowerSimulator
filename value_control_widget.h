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
    void setSteps(double singleStep, double fineStep);
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

    // UI를 현재 모드에 맞게 업데이트하는 내부 함수
    void updateUiForTuningMode();

    // 스케일 조정을 위한 변수
    double m_multiplier = 100.0;

    // 집중 모드 관련 멤버 변수들
    bool m_isFineTuningMode = false;
    double m_singleStep = 1.0;
    double m_fineStep = 0.1;
};

#endif // VALUE_CONTROL_WIDGET_H
