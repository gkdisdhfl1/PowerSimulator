#ifndef ANALYSIS_PHASOR_PAGE_H
#define ANALYSIS_PHASOR_PAGE_H

#include <QWidget>

class OneSecondSummaryData;
class QLabel;
class QVBoxLayout;
class PhasorView;

class AnalysisPhasorPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisPhasorPage(QWidget *parent = nullptr);

public slots:
    void updateSummaryData(const OneSecondSummaryData& data);

private:
    std::pair<std::array<QLabel*, 3>, std::array<QLabel*, 6>> createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit);

    PhasorView* m_phasorView;
    QWidget* m_voltageTableContainer;
    QWidget* m_currentTableContainer;
    std::array<QLabel*, 3> m_voltageNameLabels;
    std::array<QLabel*, 3> m_currentNameLabels;
    std::array<QLabel* , 6> m_voltageTable;
    std::array<QLabel*, 6> m_currentTable;
};

#endif // ANALYSIS_PHASOR_PAGE_H
