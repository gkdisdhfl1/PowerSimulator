#ifndef ANALYSIS_WAVEFORM_PAGE_H
#define ANALYSIS_WAVEFORM_PAGE_H

#include "config.h"
#include "UIconfig.h"
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
class QVBoxLayout;

class AnalysisWaveformPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisWaveformPage(QWidget *parent = nullptr);

public slots:
    void onOneSecondDataUpdated(const OneSecondSummaryData& data);

private slots:
    void onStartStopToggled(bool checked);
    void onScaleAutoToggled(bool checked);
    void onScaleTargetToggled(bool checked);
    void onScaleInClicked();
    void onScaleOutClicked();

private:
    ScaleUnit m_voltageUnit;
    ScaleUnit m_currentUnit;

    void setupUi();
    void setupTopBar(QVBoxLayout* mainLayout);
    QWidget* setupLeftPanel();
    QWidget* setupRightPanel();
    void setupChart();

    void applyScaleStep(bool zoomIn, bool voltage);
    void updateAxis(bool isVoltageAxis);
    void updatePage();

    ScaleUnit updateUnit(QValueAxis* axis, QLabel* label, int scaleIndex, bool isVoltage);

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

    OneSecondSummaryData m_lastData;
};

#endif // ANALYSIS_WAVEFORM_PAGE_H
