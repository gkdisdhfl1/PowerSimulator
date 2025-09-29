#include "one_second_summary_window.h"
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <AnalysisUtils.h>

OneSecondSummaryWindow::OneSecondSummaryWindow(QWidget *parent)
    : QWidget{parent}
{
    setupUi();
}

void OneSecondSummaryWindow::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);

    // 전체 값 그룹
    auto totalGroup = new QGroupBox("종합 (1초 평균)", this);
    auto totalLayout = new QGridLayout(totalGroup);
    totalLayout->addWidget(new QLabel("전압 RMS: ", totalGroup), 0, 0);
    m_voltageRmsLabel = new QLabel("0.000 V", totalGroup);
    totalLayout->addWidget(m_voltageRmsLabel, 0, 1);

    totalLayout->addWidget(new QLabel("전류 RMS: ", totalGroup), 1, 0);
    m_currentRmsLabel = new QLabel("0.000 V", totalGroup);
    totalLayout->addWidget(m_currentRmsLabel, 1, 1);

    totalLayout->addWidget(new QLabel("전력: ", totalGroup), 2, 0);
    m_activePowerLabel = new QLabel("0.000 W", totalGroup);
    totalLayout->addWidget(m_activePowerLabel, 2, 1);

    totalLayout->addWidget(new QLabel("누적 전력량: ", totalGroup), 3, 0);
    m_totalEnergyLabel = new QLabel("0.000000 Wh", totalGroup);
    totalLayout->addWidget(m_totalEnergyLabel, 3, 1);

    // 기본파 그룹
    auto fundGroup = new QGroupBox("기본파", this);
    auto fundLayout = new QGridLayout(fundGroup);
    fundLayout->addWidget(new QLabel("전압 RMS: ", fundGroup), 0, 0);
    m_voltageFundRmsLabel = new QLabel("0.000 V", fundGroup);
    fundLayout->addWidget(m_voltageFundRmsLabel, 0, 1);

    fundLayout->addWidget(new QLabel("전압 위상: ", fundGroup), 1, 0);
    m_voltageFundPhaseLabel = new QLabel("0.00 °", fundGroup);
    fundLayout->addWidget(m_voltageFundPhaseLabel, 1, 1);

    fundLayout->addWidget(new QLabel("전류 RMS: ", fundGroup), 2, 0);
    m_currentFundRmsLabel = new QLabel("0.000 A", fundGroup);
    fundLayout->addWidget(m_currentFundRmsLabel, 2, 1);

    fundLayout->addWidget(new QLabel("전류 위상: ", fundGroup), 3, 0);
    m_currentFundPhaseLabel = new QLabel("0.00 °", fundGroup);
    fundLayout->addWidget(m_currentFundPhaseLabel, 3, 1);

    // 고조파 그룹
    auto domGroup = new QGroupBox("고조파", this);
    auto domLayout = new QGridLayout(domGroup);

    domLayout->addWidget(new QLabel("전압 차수: ", domGroup), 0, 0);
    m_voltageHarmonicOrderLabel = new QLabel("0 차", domGroup);
    domLayout->addWidget(m_voltageHarmonicOrderLabel, 0, 1);

    domLayout->addWidget(new QLabel("전압 RMS: ", domGroup), 1, 0);
    m_voltageHarmonicRmsLabel = new QLabel("0.000 V", domGroup);
    domLayout->addWidget(m_voltageHarmonicRmsLabel, 1, 1);

    domLayout->addWidget(new QLabel("전압 위상: ", domGroup), 2, 0);
    m_voltageHarmonicPhaseLabel= new QLabel("0.00 °", domGroup);
    domLayout->addWidget(m_voltageHarmonicPhaseLabel, 2, 1);

    domLayout->addWidget(new QLabel("전류 차수: ", domGroup), 3, 0);
    m_currentHarmonicOrderLabel= new QLabel("0 차", domGroup);
    domLayout->addWidget(m_currentHarmonicOrderLabel, 3, 1);

    domLayout->addWidget(new QLabel("전류 RMS: ", domGroup), 4, 0);
    m_currentHarmonicRmsLabel = new QLabel("0.000 A", domGroup);
    domLayout->addWidget(m_currentHarmonicRmsLabel, 4, 1);

    domLayout->addWidget(new QLabel("전류 위상: ", domGroup), 5, 0);
    m_currentHarmonicPhaseLabel = new QLabel("0.00 °", domGroup);
    domLayout->addWidget(m_currentHarmonicPhaseLabel, 5, 1);

    mainLayout->addWidget(totalGroup);
    mainLayout->addWidget(fundGroup);
    mainLayout->addWidget(domGroup);
    mainLayout->addStretch();
}

void OneSecondSummaryWindow::updateData(const OneSecondSummaryData& data)
{
    // 숫자 포맷 지정: 소숫점 3자리까지
    m_voltageRmsLabel->setText(QString::number(data.totalVoltageRms, 'f', 3) + " V");
    m_currentRmsLabel->setText(QString::number(data.totalCurrentRms, 'f', 3) + " A");
    m_activePowerLabel->setText(QString::number(data.activePower, 'f', 3) + " W");
    m_totalEnergyLabel->setText(QString::number(data.totalEnergyWh, 'f', 6) + " Wh");

    // 기본파 정보 표시
    m_voltageFundRmsLabel->setText(QString::number(data.fundamentalVoltageRms, 'f', 3) + " V");
    m_voltageFundPhaseLabel->setText(QString::number(data.fundamentalVoltagePhase, 'f', 2) + " °");
    m_currentFundRmsLabel->setText(QString::number(data.fundamentalCurrentRms, 'f', 3) + " A");
    m_currentFundPhaseLabel->setText(QString::number(data.fundamentalCurrentPhase, 'f', 2) + " °");

    // 고조파 정보 표시
    m_voltageHarmonicOrderLabel->setText(QString::number(data.dominantHarmonicVoltageOrder) + " 차");
    m_voltageHarmonicRmsLabel->setText(QString::number(data.dominantHarmonicVoltageRms, 'f', 3) + " V");
    m_voltageHarmonicPhaseLabel->setText(QString::number(data.dominantHarmonicVoltagePhase, 'f', 2) + " °");

    m_currentHarmonicOrderLabel->setText(QString::number(data.dominantHarmonicCurrentOrder) + " 차");
    m_currentHarmonicRmsLabel->setText(QString::number(data.dominantHarmonicCurrentRms, 'f', 3) + " V");
    m_currentHarmonicPhaseLabel->setText(QString::number(data.dominantHarmonicCurrentPhase, 'f', 2) + " °");
}
