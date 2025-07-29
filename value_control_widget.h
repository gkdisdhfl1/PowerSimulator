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

    enum class Mode {
        Normal,
        FineTuning
    };

    void setRange(double min, double max);
    void setValue(double value);
    void setSteps(double singleStep, double fineStep);
    void setSuffix(const QString &suffix);
    double value() const; // 현재 값을 가져오는 함수

protected:
    // QWidget의 함수를 재정의하여 더블클릭 감지
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    // void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(double value);
    void onSliderMoved(int position); // 슬라이더가 움직이는 중일 때

signals:
    void valueChanged(double newValue);

private:
    Ui::ValueControlWidget *ui;

    void setMode(Mode mode); // 상태를 변경하고 UI를 업데이트하는 함수

    // UI를 현재 모드에 맞게 업데이트하는 내부 함수
    void updateUiAppearance();
    void syncSliderToValue();
    double calculateNewValue(int sliderPosition) const;
    double getFractionalPart(double value) const;


    // 집중 모드 관련 멤버 변수들
    Mode m_currentMode;

    double m_singleStep = 1.0;
    double m_fineStep = 0.01;
    double m_fineTuningRangeMin;
    double m_fineTuningRangeMax;
};

#endif // VALUE_CONTROL_WIDGET_H
