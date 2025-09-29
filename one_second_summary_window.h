#ifndef ONE_SECOND_SUMMARY_WINDOW_H
#define ONE_SECOND_SUMMARY_WINDOW_H

#include "measured_data.h"

#include <QWidget>

class QLabel;
class QGridLayout;

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

    QLabel* m_voltageRmsLabel;
    QLabel* m_currentRmsLabel;
    QLabel* m_activePowerLabel;
    QLabel* m_totalEnergyLabel;

    // 기본파 정보 표시용 라벨
    QLabel* m_voltageFundRmsLabel;
    QLabel* m_voltageFundPhaseLabel;
    QLabel* m_currentFundRmsLabel;
    QLabel* m_currentFundPhaseLabel;

    // 고조파 정보 표시용 라벨
    QLabel* m_voltageHarmonicOrderLabel;
    QLabel* m_voltageHarmonicRmsLabel;
    QLabel* m_voltageHarmonicPhaseLabel;
    QLabel* m_currentHarmonicOrderLabel;
    QLabel* m_currentHarmonicRmsLabel;
    QLabel* m_currentHarmonicPhaseLabel;

signals:
};

#endif // ONE_SECOND_SUMMARY_WINDOW_H
