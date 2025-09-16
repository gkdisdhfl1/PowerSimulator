#ifndef VALUE_CONTROL_WIDGET_H
#define VALUE_CONTROL_WIDGET_H

#include <QWidget>

class QDoubleSpinBox;
class QSlider;
class QLabel;

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
    enum class DataType {
        Double,
        Integer
    };

    void setRange(double min, double max);
    void setValue(double value);
    void setSteps(double singleStep, double fineStep);
    void setSuffix(const QString &suffix);
    void setDataType(DataType type);
    void setDecimals(int decimals);
    double value() const; // 현재 값을 가져오는 함수

protected:
    // QWidget의 함수를 재정의하여 더블클릭 감지
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onSpinBoxValueChanged(double value);
    void onSliderMoved(int position); // 슬라이더가 움직이는 중일 때

signals:
    void valueChanged(double newValue);
    void intValueChanged(int newValue);

private:
    // UI 요소들 멤버 변수
    QDoubleSpinBox* m_spinBox;
    QSlider* m_slider;
    QLabel* m_suffixLabel;

    void setupUi();
    void setMode(Mode mode); // 상태를 변경하고 UI를 업데이트하는 함수

    // UI를 현재 모드에 맞게 업데이트하는 내부 함수
    void updateUiAppearance();
    void syncSliderToValue();
    double calculateNewValue(int sliderPosition) const;
    double getFractionalPart(double value) const;


    // 집중 모드 관련 멤버 변수들
    Mode m_currentMode;
    DataType m_dataType = DataType::Double;

    double m_singleStep = 1.0;
    double m_fineStep = 0.01;
    double m_fineTuningRangeMin;
    double m_fineTuningRangeMax;
};

#endif // VALUE_CONTROL_WIDGET_H
