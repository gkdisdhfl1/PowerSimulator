#include "one_second_summary_window.h"
#include "config.h"
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

    // 각 행의 제목 아이템 설정
    m_tableWidget->setItem(Row::TotalRmsA, Col::Title, new QTableWidgetItem("RMS (A) (V/A)"));
    m_tableWidget->setItem(Row::TotalRmsB, Col::Title, new QTableWidgetItem("RMS (B) (V/A)"));
    m_tableWidget->setItem(Row::TotalRmsC, Col::Title, new QTableWidgetItem("RMS (C) (V/A)"));
    m_tableWidget->setItem(Row::ActivePowerA, Col::Title, new QTableWidgetItem("유효전력 (A) (W)"));
    m_tableWidget->setItem(Row::ActivePowerB, Col::Title, new QTableWidgetItem("유효전력 (B) (W)"));
    m_tableWidget->setItem(Row::ActivePowerC, Col::Title, new QTableWidgetItem("유효전력 (C) (W)"));
    m_tableWidget->setItem(Row::TotalActivePower, Col::Title, new QTableWidgetItem("총 유효전력 (W)"));
    m_tableWidget->setItem(Row::TotalEnergy, Col::Title, new QTableWidgetItem("누적전력량 (Wh)"));
    m_tableWidget->setItem(Row::FundamentalRms, Col::Title, new QTableWidgetItem("RMS (V/A)"));
    m_tableWidget->setItem(Row::FundamentalPhase, Col::Title, new QTableWidgetItem("위상 (°)"));
    m_tableWidget->setItem(Row::DominantOrder, Col::Title, new QTableWidgetItem("차수"));
    m_tableWidget->setItem(Row::DominantRms, Col::Title, new QTableWidgetItem("RMS (V/A)"));
    m_tableWidget->setItem(Row::DominantPhase, Col::Title, new QTableWidgetItem("위상 (°)"));

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
    m_tableWidget->item(Row::TotalEnergy, Col::Voltage)->setText(QString::number(data.totalEnergyWh, 'f', 6));

    // 기본파 정보 표시
    m_tableWidget->item(Row::FundamentalRms, Col::Voltage)->setText(QString::number(data.fundamentalVoltage[0].rms, 'f', 3));
    m_tableWidget->item(Row::FundamentalRms, Col::Current)->setText(QString::number(data.fundamentalCurrent[0].rms, 'f', 3));
    m_tableWidget->item(Row::FundamentalPhase, Col::Voltage)->setText(QString::number(utils::radiansToDegrees(data.fundamentalVoltage[0].phase), 'f', 2));
    m_tableWidget->item(Row::FundamentalPhase, Col::Current)->setText(QString::number(utils::radiansToDegrees(data.fundamentalCurrent[0].phase), 'f', 2));

    // 고조파 정보 표시
    m_tableWidget->item(Row::DominantOrder, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltageOrder));
    m_tableWidget->item(Row::DominantOrder, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentOrder));
    m_tableWidget->item(Row::DominantRms, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltageRms, 'f', 3));
    m_tableWidget->item(Row::DominantRms, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentRms, 'f', 3));
    m_tableWidget->item(Row::DominantPhase, Col::Voltage)->setText(QString::number(data.dominantHarmonicVoltagePhase, 'f', 2));
    m_tableWidget->item(Row::DominantPhase, Col::Current)->setText(QString::number(data.dominantHarmonicCurrentPhase, 'f', 2));
}
