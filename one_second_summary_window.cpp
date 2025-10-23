#include "one_second_summary_window.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <analysis_utils.h>
#include <QPalette>
#include <QHeaderView>

OneSecondSummaryWindow::OneSecondSummaryWindow(QWidget *parent)
    : QWidget{parent}
{
    setupUi();
}

void OneSecondSummaryWindow::setupUi()
{
    m_tableWidget = new QTableWidget(Row::RowCount, Col::ColumnCount, this);
    m_tableWidget->setHorizontalHeaderLabels({"구분", "전압", "전류"});

    // 그룹 헤더 설정
    auto setupHeaderRow = [&](int row, const QString& text) {
        auto* headerItem = new QTableWidgetItem(text);
        headerItem->setBackground(palette().color(QPalette::Window)); // 배경색
        headerItem->setFlags(Qt::ItemIsEnabled);
        m_tableWidget->setItem(row, 0, headerItem);
        m_tableWidget->setSpan(row, 0, 1, Col::ColumnCount); // 열 병합
    };
    setupHeaderRow(Row::HeaderTotal, "▼ 전체 분석");
    setupHeaderRow(Row::HeaderFundamental, "▼ 기분파 분석");
    setupHeaderRow(Row::HeaderDominant, "▼ 고조파 분석");
    setupHeaderRow(Row::HeaderSymmetrical, "▼ 대칭 성분 분석");

    // 각 행의 제목 아이템 설정
    m_tableWidget->setItem(Row::TotalRmsA, Col::Title, new QTableWidgetItem("RMS (A) (V/A)"));
    m_tableWidget->setItem(Row::TotalRmsB, Col::Title, new QTableWidgetItem("RMS (B) (V/A)"));
    m_tableWidget->setItem(Row::TotalRmsC, Col::Title, new QTableWidgetItem("RMS (C) (V/A)"));
    m_tableWidget->setItem(Row::ActivePowerA, Col::Title, new QTableWidgetItem("유효전력 (A) (W)"));
    m_tableWidget->setItem(Row::ActivePowerB, Col::Title, new QTableWidgetItem("유효전력 (B) (W)"));
    m_tableWidget->setItem(Row::ActivePowerC, Col::Title, new QTableWidgetItem("유효전력 (C) (W)"));
    m_tableWidget->setItem(Row::TotalActivePower, Col::Title, new QTableWidgetItem("총 유효전력 (W)"));
    m_tableWidget->setItem(Row::ApparentPowerA, Col::Title, new QTableWidgetItem("피상전력 (A) (VA)"));
    m_tableWidget->setItem(Row::ApparentPowerB, Col::Title, new QTableWidgetItem("피상전력 (B) (VA)"));
    m_tableWidget->setItem(Row::ApparentPowerC, Col::Title, new QTableWidgetItem("피상전력 (C) (VA)"));
    m_tableWidget->setItem(Row::TotalApparentPower, Col::Title, new QTableWidgetItem("총 피상전력 (VA)"));
    m_tableWidget->setItem(Row::PowerFactorA, Col::Title, new QTableWidgetItem("역률 (A)"));
    m_tableWidget->setItem(Row::PowerFactorB, Col::Title, new QTableWidgetItem("역률 (B)"));
    m_tableWidget->setItem(Row::PowerFactorC, Col::Title, new QTableWidgetItem("역률 (C)"));
    m_tableWidget->setItem(Row::TotalPowerFactor, Col::Title, new QTableWidgetItem("총 역률"));
    m_tableWidget->setItem(Row::TotalEnergy, Col::Title, new QTableWidgetItem("누적전력량 (Wh)"));
    m_tableWidget->setItem(Row::FundamentalRms, Col::Title, new QTableWidgetItem("RMS (V/A)"));
    m_tableWidget->setItem(Row::FundamentalPhase, Col::Title, new QTableWidgetItem("위상 (°)"));
    m_tableWidget->setItem(Row::ThdA, Col::Title, new QTableWidgetItem("THD (A) (%)"));
    m_tableWidget->setItem(Row::ThdB, Col::Title, new QTableWidgetItem("THD (B) (%)"));
    m_tableWidget->setItem(Row::ThdC, Col::Title, new QTableWidgetItem("THD (C) (%)"));
    m_tableWidget->setItem(Row::SystemThd, Col::Title, new QTableWidgetItem("Ststem THD (%)"));
    m_tableWidget->setItem(Row::DominantOrder, Col::Title, new QTableWidgetItem("차수"));
    m_tableWidget->setItem(Row::DominantRms, Col::Title, new QTableWidgetItem("RMS (V/A)"));
    m_tableWidget->setItem(Row::DominantPhase, Col::Title, new QTableWidgetItem("위상 (°)"));
    m_tableWidget->setItem(Row::ZeroSequence, Col::Title, new QTableWidgetItem("영상분 (V/A)"));
    m_tableWidget->setItem(Row::PositiveSequence, Col::Title, new QTableWidgetItem("정상분 (V/A)"));
    m_tableWidget->setItem(Row::NegativeSequence, Col::Title, new QTableWidgetItem("역상분 (V/A)"));

    // 초기값 아이템 생성 및 설정
    for(int row = 0; row < Row::RowCount; ++row) {
        // 제목 셀 설정
        if(m_tableWidget->item(row, Col::Title)) {
            m_tableWidget->item(row, Col::Title)->setFlags(Qt::ItemIsEnabled);
        }
        // 값 셀 설정
        for(int col = 1; col < Col::ColumnCount; ++col) {
            if(!m_tableWidget->item(row, col)) {
                auto valueItem = new QTableWidgetItem("0.0");
                valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignCenter);
                valueItem->setFlags(Qt::ItemIsEnabled);
                m_tableWidget->setItem(row, col, valueItem);
            }
        }
    }

    // 유효전력, 누적전력량의 전류 셀은 비워둠
    m_tableWidget->item(Row::ActivePowerA, Col::Current)->setText("");
    m_tableWidget->item(Row::ActivePowerB, Col::Current)->setText("");
    m_tableWidget->item(Row::ActivePowerC, Col::Current)->setText("");
    m_tableWidget->item(Row::TotalActivePower, Col::Current)->setText("");
    m_tableWidget->item(Row::TotalEnergy, Col::Current)->setText("");

    // 테이블 외형 설정
    m_tableWidget->verticalHeader()->setVisible(false);

    // 첫 번째 '구분' 열은 내용에 맞게 크기 조절
    m_tableWidget->horizontalHeader()->setSectionResizeMode(Col::Title, QHeaderView::ResizeToContents);
    // '전압', '전류' 열은 남은 공간을 균등하게 차지하도록 설정
    m_tableWidget->horizontalHeader()->setSectionResizeMode(Col::Voltage, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(Col::Current, QHeaderView::Stretch);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tableWidget);
    setLayout(mainLayout);
}

void OneSecondSummaryWindow::updateData(const OneSecondSummaryData& data)
{

    // 숫자 포맷 지정: 소숫점 3자리까지
    m_tableWidget->item(Row::TotalRmsA, Col::Voltage)->setText(QString::number(data.totalVoltageRms.a, 'f', 3));
    m_tableWidget->item(Row::TotalRmsA, Col::Current)->setText(QString::number(data.totalCurrentRms.a, 'f', 3));
    m_tableWidget->item(Row::TotalRmsB, Col::Voltage)->setText(QString::number(data.totalVoltageRms.b, 'f', 3));
    m_tableWidget->item(Row::TotalRmsB, Col::Current)->setText(QString::number(data.totalCurrentRms.b, 'f', 3));
    m_tableWidget->item(Row::TotalRmsC, Col::Voltage)->setText(QString::number(data.totalVoltageRms.c, 'f', 3));
    m_tableWidget->item(Row::TotalRmsC, Col::Current)->setText(QString::number(data.totalCurrentRms.c, 'f', 3));
    m_tableWidget->item(Row::ActivePowerA, Col::Voltage)->setText(QString::number(data.activePower.a, 'f', 3));
    m_tableWidget->item(Row::ActivePowerB, Col::Voltage)->setText(QString::number(data.activePower.b, 'f', 3));
    m_tableWidget->item(Row::ActivePowerC, Col::Voltage)->setText(QString::number(data.activePower.c, 'f', 3));
    m_tableWidget->item(Row::TotalActivePower, Col::Voltage)->setText(QString::number(data.totalActivePower, 'f', 3));
    m_tableWidget->item(Row::ApparentPowerA, Col::Voltage)->setText(QString::number(data.apparentPower.a, 'f', 3));
    m_tableWidget->item(Row::ApparentPowerB, Col::Voltage)->setText(QString::number(data.apparentPower.b, 'f', 3));
    m_tableWidget->item(Row::ApparentPowerC, Col::Voltage)->setText(QString::number(data.apparentPower.c, 'f', 3));
    m_tableWidget->item(Row::TotalApparentPower, Col::Voltage)->setText(QString::number(data.totalApparentPower, 'f', 3));
    m_tableWidget->item(Row::PowerFactorA, Col::Voltage)->setText(QString::number(data.powerFactor.a, 'f', 3));
    m_tableWidget->item(Row::PowerFactorB, Col::Voltage)->setText(QString::number(data.powerFactor.b, 'f', 3));
    m_tableWidget->item(Row::PowerFactorC, Col::Voltage)->setText(QString::number(data.powerFactor.c, 'f', 3));
    m_tableWidget->item(Row::TotalPowerFactor, Col::Voltage)->setText(QString::number(data.totalPowerFactor, 'f', 3));
    m_tableWidget->item(Row::TotalEnergy, Col::Voltage)->setText(QString::number(data.totalEnergyWh, 'f', 6));

    // 기본파 정보 표시
    m_tableWidget->item(Row::FundamentalRms, Col::Voltage)->setText(QString::number(data.fundamentalVoltageRms, 'f', 3));
    m_tableWidget->item(Row::FundamentalRms, Col::Current)->setText(QString::number(data.fundamentalCurrentRms, 'f', 3));
    m_tableWidget->item(Row::FundamentalPhase, Col::Voltage)->setText(QString::number(data.fundamentalVoltagePhase, 'f', 2));
    m_tableWidget->item(Row::FundamentalPhase, Col::Current)->setText(QString::number(data.fundamentalCurrentPhase, 'f', 2));
    m_tableWidget->item(Row::ThdA, Col::Voltage)->setText(QString::number(data.voltageThd.a, 'f', 3));
    m_tableWidget->item(Row::ThdA, Col::Current)->setText(QString::number(data.currentThd.a, 'f', 3));
    m_tableWidget->item(Row::ThdB, Col::Voltage)->setText(QString::number(data.voltageThd.b, 'f', 3));
    m_tableWidget->item(Row::ThdB, Col::Current)->setText(QString::number(data.currentThd.b, 'f', 3));
    m_tableWidget->item(Row::ThdC, Col::Voltage)->setText(QString::number(data.voltageThd.c, 'f', 3));
    m_tableWidget->item(Row::ThdC, Col::Current)->setText(QString::number(data.currentThd.c, 'f', 3));
    m_tableWidget->item(Row::SystemThd, Col::Voltage)->setText(QString::number(data.systemVoltageThd, 'f', 3));
    m_tableWidget->item(Row::SystemThd, Col::Current)->setText(QString::number(data.systemCurrentThd, 'f', 3));

    // 고조파 정보 표시
    m_tableWidget->item(Row::DominantOrder, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltageOrder));
    m_tableWidget->item(Row::DominantOrder, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentOrder));
    m_tableWidget->item(Row::DominantRms, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltageRms, 'f', 3));
    m_tableWidget->item(Row::DominantRms, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentRms, 'f', 3));
    m_tableWidget->item(Row::DominantPhase, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltagePhase, 'f', 2));
    m_tableWidget->item(Row::DominantPhase, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentPhase, 'f', 2));

    // 대칭 정보 표시
    m_tableWidget->item(Row::ZeroSequence, Col::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.zero.magnitude, 'f', 3));
    m_tableWidget->item(Row::ZeroSequence, Col::Current)->setText(QString::number(data.currentSymmetricalComponents.zero.magnitude, 'f', 3));
    m_tableWidget->item(Row::PositiveSequence, Col::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.positive.magnitude, 'f', 3));
    m_tableWidget->item(Row::PositiveSequence, Col::Current)->setText(QString::number(data.currentSymmetricalComponents.positive.magnitude, 'f', 3));
    m_tableWidget->item(Row::NegativeSequence, Col::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.negative.magnitude, 'f', 3));
    m_tableWidget->item(Row::NegativeSequence, Col::Current)->setText(QString::number(data.currentSymmetricalComponents.negative.magnitude, 'f', 3));
}
