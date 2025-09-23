#ifndef PHASOR_VIEW_H
#define PHASOR_VIEW_H

#include <QWidget>
#include <deque>
#include "measured_data.h"

class QCheckBox;
class QLabel;

class PhasorView : public QWidget
{
    Q_OBJECT
public:
    explicit PhasorView(QWidget *parent = nullptr);

public slots:
    void updateData(const std::deque<MeasuredData>& data);

protected:
    // 위젯을 다시 그릴 때 호출되는 이벤트 핸들러
    void paintEvent(QPaintEvent *event) override;

private:
    struct PhasorInfo {
        QPointF components{0, 0};
        double magnitude = 0.0;
        double phaseDegrees = 0.0;
    };

    double getVoltageLength(double magnitude, double maxRadius);
    double getCurrentLength(double magnitude, double maxRadius);
    void drawPhasor(QPainter& painter, const PhasorInfo& phasor, const QColor& color, double radius);

    // UI 요소
    QWidget* m_controlContainer;
    QCheckBox* m_totalVoltageCheck;
    QCheckBox* m_totalCurrentCheck;
    QCheckBox* m_fundVoltageCheck;
    QCheckBox* m_fundCurrentCheck;
    QLabel* m_voltageInfoLabel;
    QLabel* m_currentInfoLabel;

    // 표시할 데이터
    PhasorInfo m_totalVoltage;
    PhasorInfo m_totalCurrent;
    PhasorInfo m_fundamentalVoltage;
    PhasorInfo m_fundamentalCurrent;

};

#endif // PHASOR_VIEW_H
