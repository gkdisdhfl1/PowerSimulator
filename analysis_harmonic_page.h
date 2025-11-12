#ifndef ANALYSIS_HARMONIC_PAGE_H
#define ANALYSIS_HARMONIC_PAGE_H

#include "config.h"
#include "measured_data.h"
#include <QWidget>

class QVBoxLayout;
class QPushButton;
class QButtonGroup;
class QComboBox;
class QCheckBox;
class QStackedWidget;
class QLabel;

class QValueAxis;
class QBarSeries;
class QBarSet;
class QChart;
class QChartView;
class QCategoryAxis;

class AnalysisHarmonicPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisHarmonicPage(QWidget *parent = nullptr);

public slots:
    void onOneSecondDataUpdated(const OneSecondSummaryData& data);

private slots:
    void onDisplayTypeChanged(int id);
    void onScaleAutoToggled(bool checked);
    void onScaleInClicked();
    void onScaleOutClicked();

private:
    void setupUi();
    void setupTopBar(QVBoxLayout* mainLayout); // 상단 바
    void setupControlBar(QVBoxLayout* mainLayout);
    void updateChartAxis();

    QWidget* createGraphView();
    QWidget* createTextView();


    // UI 멤버 변수
    QPushButton* m_voltageButton;
    QPushButton* m_currentButton;
    QPushButton* m_autoScaleButton;
    QButtonGroup* m_buttonGroup; // 토글 버튼 그룹
    QCheckBox* m_fundCheckBox;
    QComboBox* m_dataTypeComboBox;
    QComboBox* m_viewTypeComboBox;
    std::array<QCheckBox*, 3> m_phaseCheckBoxes;
    QStackedWidget* m_contentStack;
    std::array<QLabel*, 3> m_thdValueLabels;
    std::array<QLabel*, 3> m_fundValueLabels;

    // 스케일링 및 그래프 관련 멤버 변수
    ScaleUnit m_scaleUnit;
    int m_scaleIndex = 0;
    bool m_isAutoScaling = true;

    QChart* m_chart;
    QChartView* m_chartView;
    QValueAxis* m_axisY;
    QCategoryAxis* m_axisX;
    QLabel* m_unitLabel;
    QBarSeries* m_barSeries;
    std::array<QBarSet*, 3> m_barSets; // A B C상
};

#endif // ANALYSIS_HARMONIC_PAGE_H
