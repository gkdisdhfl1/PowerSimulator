#ifndef ONE_SECOND_SUMMARY_WINDOW_H
#define ONE_SECOND_SUMMARY_WINDOW_H

#include "measured_data.h"

#include <QWidget>

class QTableWidget;

// 테이블의 행 인덱스를 상수로 정의
namespace Row {
enum
{
    HeaderTotal,
    TotalRmsA,
    TotalRmsB,
    TotalRmsC,
    ActivePowerA,
    ActivePowerB,
    ActivePowerC,
    TotalActivePower,
    ApparentPowerA,
    ApparentPowerB,
    ApparentPowerC,
    TotalApparentPower,
    PowerFactorA,
    PowerFactorB,
    PowerFactorC,
    TotalPowerFactor,
    TotalEnergy,
    HeaderFundamental,
    FundamentalRms,
    FundamentalPhase,
    ThdA,
    ThdB,
    ThdC,
    SystemThd,
    HeaderDominant,
    DominantOrder,
    DominantRms,
    DominantPhase,
    RowCount
};
}
namespace Col {
enum {
    Title,
    Voltage,
    Current,
    ColumnCount
};
}

class OneSecondSummaryWindow : public QWidget
{
    Q_OBJECT
public:
    explicit OneSecondSummaryWindow(QWidget *parent = nullptr);

public slots:
    // SimulationEngine의 신호를 받아 UI를 업데이트할 슬롯
    void updateData(const OneSecondSummaryData& data);

private:
    void setupUi();

    QTableWidget* m_tableWidget;
};

#endif // ONE_SECOND_SUMMARY_WINDOW_H
