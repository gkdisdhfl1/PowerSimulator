#ifndef ONE_SECOND_SUMMARY_WINDOW_H
#define ONE_SECOND_SUMMARY_WINDOW_H

#include "measured_data.h"

#include <QWidget>

class QTableWidget;

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
