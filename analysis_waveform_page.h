#ifndef ANALYSIS_WAVEFORM_PAGE_H
#define ANALYSIS_WAVEFORM_PAGE_H

#include "measured_data.h"

#include <QWidget>

class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;

class QPushButton;
class QCheckBox;
class QButtonGroup;
class QLabel;

static const std::vector<double> RANGE_TABLE = {
    0.004, 0.008, 0.020, 0.040, 0.080,
    0.200, 0.400, 0.800,
    2.0, 4.0, 8.0, 20.0 , 40.0, 80.0,
    200.0, 400.0, 800.0, 2000.0
};

class AnalysisWaveformPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisWaveformPage(QWidget *parent = nullptr);

public slots:
    void updateWaveformData(const OneSecondSummaryData& data);

private slots:
    void onStartStopToggled(bool checked);
    void onScaleAutoToggled(bool checked);
    void onScaleTargetToggled(bool checked);
    void onScaleInClicked();
    void onScaleOutClicked();

private:
    enum class ScaleUnit { Milli, Base, Kilo };
    ScaleUnit m_voltageUnit;
    ScaleUnit m_currentUnit;

    void setupUi();
    void applyScaleStep(bool zoomIn, bool voltage);
    void updateScaleUnit(double range, bool voltage);
    double scaleValue(double value, ScaleUnit unit);
    void updateAxis(bool isVoltageAxis);

    // UI 위젯
    QPushButton* m_startButton;
    std::array<QCheckBox*, 3> m_voltagePhaseChecks;
    std::array<QCheckBox*, 3> m_currentPhaseChecks;
    std::array<QPushButton*, 4> m_scaleButtons;
    QButtonGroup* m_scaleButtonGroup;

    // 차트 관련
    QChart* m_chart;
    QChartView* m_chartView;
    QValueAxis* m_axisX;
    QValueAxis* m_axisV;
    QValueAxis* m_axisA;
    QLabel* m_voltageScaleLabel;
    QLabel* m_currentScaleLabel;

    // 3상 전압/전류 시리즈
    std::array<QLineSeries*, 3> m_voltageSeries;
    std::array<QLineSeries*, 3> m_currentSeries;

    bool m_isUpdating; // 시작/정지 상태
    bool m_isAutoScaling;
    bool m_isTargetVoltage;
    int m_voltageScaleIndex = 0;
    int m_currentScaleIndex = 0;
};

#endif // ANALYSIS_WAVEFORM_PAGE_H
