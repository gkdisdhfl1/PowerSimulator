#ifndef ANALYSIS_PHASOR_PAGE_H
#define ANALYSIS_PHASOR_PAGE_H

#include "analysis_utils.h"
#include "measured_data.h"
#include <QWidget>
#include <QLabel>

class QCheckBox;
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

    template <typename T>
    void updatePhasorTable(std::array<QLabel*, 6>& table, const GenericPhaseData<T>& phaseData)
    {
        for(int i{0}; i < 3; ++i) {
            const auto& data = AnalysisUtils::getPhaseComponent(i, phaseData);
            table[i * 2 + 0]->setText(QString::number(data.rms, 'f', 3));
            table[i * 2 + 1]->setText(QString::number(utils::radiansToDegrees(data.phase), 'f', 1) + "°");
        }
    }

    TableWidgets createPhasorTable(QVBoxLayout* layout, const QString& title, const QStringList& labels, const QString& unit);
    void createTopBar(QVBoxLayout* mainLayout);
    void createContentArea(QHBoxLayout* contentLayout);
    void createTableSection(QVBoxLayout* tableLayout);
    void setupConnections();

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
    QButtonGroup* m_voltageModeGroup = nullptr; // 라디오 동작용
    QCheckBox* m_voltageCheck;
    QCheckBox* m_currentCheck;

};

#endif // ANALYSIS_PHASOR_PAGE_H
