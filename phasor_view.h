#ifndef PHASOR_VIEW_H
#define PHASOR_VIEW_H

#include <QWidget>
#include "measured_data.h"

class QCheckBox;
class QLabel;
class QGridLayout;

struct DrawingContext {
        QPointF origin;
        double maxRadius;
        double voltageRadius; //
        double currentBaseRadius;
        double voltageAnnulusHeight;

        // 생성자에서 모든 계산을 한번에 수행
        DrawingContext(const QRect& widgetRect, int topMargin);
};

class PhasorView : public QWidget
{
    Q_OBJECT
public:
    explicit PhasorView(QWidget *parent = nullptr);

public slots:
    void updateData(const std::array<HarmonicAnalysisResult, 3>& fundamentalVoltage,
                    const std::array<HarmonicAnalysisResult, 3>& fundamentalCurrent,
                    const std::vector<HarmonicAnalysisResult>& voltageHarmonics,
                    const std::vector<HarmonicAnalysisResult>& currentHarmonics);
    void onVisibilityChanged(int type, bool isVisible);

protected:
    // 위젯을 다시 그릴 때 호출되는 이벤트 핸들러
    void paintEvent(QPaintEvent *event) override;

private:
    struct PhasorInfo {
        QPointF components{0, 0};
        double magnitude = 0.0;
        double phaseDegrees = 0.0;
    };

    void drawGuideLines(QPainter& painter, const DrawingContext& ctx) const;
    double getPhasorDisplayLength(double ratio, const DrawingContext& ctx, bool isVoltage) const;
    void drawPhasor(QPainter& painter, const PhasorInfo& phasor, const QColor& color, double radius);


    // UI 요소
    QLabel* m_voltageInfoLabel;
    QLabel* m_currentInfoLabel;

    std::array<bool, 8> m_phasorIsVisible;
    QGridLayout* m_infoLayout;

    // 표시할 데이터
    std::array<PhasorInfo, 3> m_fundVoltage;
    std::array<PhasorInfo, 3> m_fundCurrent;
    PhasorInfo m_harmonicVoltage;
    PhasorInfo m_harmonicCurrent;

};

#endif // PHASOR_VIEW_H
