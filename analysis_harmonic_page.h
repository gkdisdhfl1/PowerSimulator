#ifndef ANALYSIS_HARMONIC_PAGE_H
#define ANALYSIS_HARMONIC_PAGE_H

#include "UIconfig.h"
#include "measured_data.h"
#include <QStyledItemDelegate>
#include <QWidget>

class QVBoxLayout;
class QPushButton;
class QButtonGroup;
class QComboBox;
class QCheckBox;
class QStackedWidget;
class QLabel;
class QTableWidget;

class QValueAxis;
class QBarSeries;
class QBarSet;
class QChart;
class QChartView;
class QCategoryAxis;

class HarmonicTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HarmonicTextDelegate(QObject* parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

struct HarmonicDataSources {
    const std::vector<HarmonicAnalysisResult>* harmonics = nullptr;
    const PhaseData* totalRms = nullptr;
    const GenericPhaseData<HarmonicAnalysisResult>* fundamental = nullptr;
    int dataTypeIndex = 0;
};

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
    void onFundVisibleChanged(bool checked);
    void onPhaseVisibleChanged(int id, bool checked);
    void onViewTypeChanged(int index);

private:
    void setupUi();
    void setupTopBar(QVBoxLayout* mainLayout); // 상단 바
    void setupControlBar(QVBoxLayout* mainLayout);
    void updateChartAxis();
    void updateGraph();
    void updateText();
    void updateInfoLabels();
    HarmonicDataSources getCurrentDataSources(int phaseIndex) const;
    double calculateRawValue(const HarmonicDataSources& sources, int order, int phaseIndex) const;

    QWidget* createGraphView();
    QWidget* createTextView();


    // UI 멤버 변수
    QButtonGroup* m_buttonGroup; // 토글 버튼 그룹
    QButtonGroup* m_scaleButtonGroup;
    QButtonGroup* m_phaseButtonGroup;
    QPushButton* m_voltageButton;
    QPushButton* m_currentButton;
    QCheckBox* m_fundCheckBox;
    QComboBox* m_dataTypeComboBox;
    QComboBox* m_viewTypeComboBox;
    std::array<QCheckBox*, 3> m_phaseCheckBoxes;
    QStackedWidget* m_contentStack;
    QTableWidget* m_textTable;
    std::array<QLabel*, 3> m_thdValueLabels;
    std::array<QLabel*, 3> m_fundValueLabels;
    std::array<QLabel*, 3> m_fundUnitLabels;
    std::array<QPushButton*, 3> m_scaleButtons;

    // 스케일링 및 그래프 관련 멤버 변수
    ScaleUnit m_scaleUnit;
    int m_scaleIndex = 0;
    bool m_isAutoScaling = true;
    bool m_isFundVisible = true;
    std::array<bool, 3> m_isPhaseVisible = {true, true, true};

    QChart* m_chart;
    QChartView* m_chartView;
    QValueAxis* m_axisY;
    QCategoryAxis* m_axisX;
    QLabel* m_unitLabel;
    QBarSeries* m_barSeries;
    std::array<QBarSet*, 3> m_barSets; // A B C상

    OneSecondSummaryData m_lastSummaryData;
    bool m_hasData = false;
};

#endif // ANALYSIS_HARMONIC_PAGE_H
