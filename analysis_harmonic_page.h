#ifndef ANALYSIS_HARMONIC_PAGE_H
#define ANALYSIS_HARMONIC_PAGE_H

#include <QWidget>

class QVBoxLayout;
class QPushButton;
class QButtonGroup;

class AnalysisHarmonicPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisHarmonicPage(QWidget *parent = nullptr);

private:
    void setupUi();
    void setupTopBar(QVBoxLayout* mainLayout); // 상단 바

    // UI 멤버 변수
    QPushButton* m_voltageButton;
    QPushButton* m_currentButton;
    QButtonGroup* m_buttonGroup; // 토글 버튼 그룹
};

#endif // ANALYSIS_HARMONIC_PAGE_H
