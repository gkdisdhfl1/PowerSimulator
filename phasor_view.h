#ifndef PHASOR_VIEW_H
#define PHASOR_VIEW_H

#include <QWidget>
#include <deque>
#include "measured_data.h"

class QCheckBox;

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
    void drawPhasor(QPainter& painter, const QPointF& phasor, const QColor& color, double radius);

    // UI 요소
    QCheckBox* m_voltageVisibleCheck;
    QCheckBox* m_currentVisibleCheck;

    // 표시할 데이터
    QPointF m_voltagePhasor;
    QPointF m_currentPhasor;

};

#endif // PHASOR_VIEW_H
