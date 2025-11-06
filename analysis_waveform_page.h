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

class AnalysisWaveformPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisWaveformPage(QWidget *parent = nullptr);

public slots:
    void updateWaveformData(const OneSecondSummaryData& data);

private:
    void setupUi();

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

    // 3상 전압/전류 시리즈
    std::array<QLineSeries*, 3> m_voltageSeries;
    std::array<QLineSeries*, 3> m_currentSeries;

    bool m_isUpdating; // 시작/정지 상태
};

#endif // ANALYSIS_WAVEFORM_PAGE_H
