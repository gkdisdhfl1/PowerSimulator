#ifndef ANALYSIS_PHASOR_PAGE_H
#define ANALYSIS_PHASOR_PAGE_H

#include "measured_data.h"
#include <QWidget>

class OneSecondSummaryData;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class PhasorView;
class QPushButton;
class QButtonGroup;

class AnalysisPhasorPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisPhasorPage(QWidget *parent = nullptr);

public slots:
    void updateSummaryData(const OneSecondSummaryData& data);

private:
    struct TableWidgets {
        std::array<QLabel*, 3> nameLabels;
        std::array<QLabel*, 6> valueLabels;
        QHBoxLayout* headerLayout; // 제목 줄 레이아웃
    };
    TableWidgets createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit);

    PhasorView* m_phasorView;
    QWidget* m_voltageTableContainer;
    QWidget* m_currentTableContainer;
    std::array<QLabel*, 3> m_voltageNameLabels;
    std::array<QLabel*, 3> m_currentNameLabels;
    std::array<QLabel* , 6> m_voltageTable;
    std::array<QLabel*, 6> m_currentTable;
    OneSecondSummaryData m_lastSummaryData; // 데이터 캐싱
    QPushButton* m_vlnButton = nullptr;
    QPushButton* m_vllButton = nullptr;
    QButtonGroup* m_voltageModeGroup; // 라디오 동작용

};

#endif // ANALYSIS_PHASOR_PAGE_H
