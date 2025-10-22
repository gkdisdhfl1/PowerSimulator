#ifndef ADDITIONAL_METRICS_WINDOW_H
#define ADDITIONAL_METRICS_WINDOW_H

#include <QWidget>
#include "measured_data.h"

class QTableWidget;

namespace MetricsRow {
enum {
    ResidualRms,
    Thd,
    ApparentPower,
    ReactivePower,
    powerFactor,
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
    void updateData(const AdditionalMetricsData& data);
private:
    void setupUi();

    QTableWidget* m_tableWidget;
};

#endif // ADDITIONAL_METRICS_WINDOW_H
