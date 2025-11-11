#ifndef ANALYSIS_HARMONIC_PAGE_H
#define ANALYSIS_HARMONIC_PAGE_H

#include <QWidget>

class QVBoxLayout;
class QPushButton;
class QButtonGroup;
class QComboBox;
class QCheckBox;
class QStackedWidget;
class QLabel;

class AnalysisHarmonicPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisHarmonicPage(QWidget *parent = nullptr);

private slots:
    void onDisplayTypeChanged(int id);

private:
    void setupUi();
    void setupTopBar(QVBoxLayout* mainLayout); // 상단 바
    void setupControlBar(QVBoxLayout* mainLayout);

    QWidget* createGraphView();
    QWidget* createTextView();

    // UI 멤버 변수
    QPushButton* m_voltageButton;
    QPushButton* m_currentButton;
    QButtonGroup* m_buttonGroup; // 토글 버튼 그룹
    QCheckBox* m_fundCheckBox;
    QComboBox* m_dataTypeComboBox;
    QComboBox* m_viewTypeComboBox;
    std::array<QCheckBox*, 3> m_phaseCheckBoxes;
    QStackedWidget* m_contentStack;
    std::array<QLabel*, 3> m_thdValueLabels;
    std::array<QLabel*, 3> m_fundValueLabels;
};

#endif // ANALYSIS_HARMONIC_PAGE_H
