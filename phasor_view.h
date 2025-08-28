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

    void drawPhasor(QPainter& painter, const PhasorInfo& phasor, const QColor& color, double radius);

    // UI 요소
    QWidget* m_controlContainer;
    QCheckBox* m_voltageVisibleCheck;
    QCheckBox* m_currentVisibleCheck;
    QLabel* m_voltageInfoLabel;
    QLabel* m_currentInfoLabel;

    // 표시할 데이터
    PhasorInfo m_voltage;
    PhasorInfo m_current;

};

#endif // PHASOR_VIEW_H
