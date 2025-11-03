#ifndef ADDITIONAL_METRICS_WINDOW_H
#define ADDITIONAL_METRICS_WINDOW_H

#include <QWidget>
#include "measured_data.h"

class QTableWidget;

namespace MetricsRow {
enum {
    ResidualRms,
    ResidualFundamental,
    HeaderApparent,
    ApparentPowerA,
    ApparentPowerB,
    ApparentPowerC,
    TotalApparentPower,
    HeaderReactivePower,
    ReactivePowerA,
    ReactivePowerB,
    ReactivePowerC,
    TotalReactivePower,
    HeaderPowerFactor,
    PowerFactorA,
    PowerFactorB,
    PowerFactorC,
    TotalPowerFactor,
    HeaderThd,
    ThdA,
    ThdB,
    ThdC,
    HeaderSymmetrical,
    ZeroSequence,
    PositiveSequence,
    NegativeSequence,
    HeaderNemaUnbalance,
    NemaUnbalance,
    HeaederU0U2Unbalance,
    U0Unbalance,
    U2Unbalance,
    RowCount
    };
}
namespace MetricsCol {
enum {
    Title,
    Voltage,
    Current,
    ColumnCount
};
}

class AdditionalMetricsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AdditionalMetricsWindow(QWidget* parent = nullptr);

public slots:
    void updateData(const OneSecondSummaryData& data);
private:
    void setupUi();

    QTableWidget* m_tableWidget;
};

#endif // ADDITIONAL_METRICS_WINDOW_H
