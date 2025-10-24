#include "additional_metrics_window.h"
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>

AdditionalMetricsWindow::AdditionalMetricsWindow(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void AdditionalMetricsWindow::setupUi()
{
    m_tableWidget = new QTableWidget(MetricsRow::RowCount, MetricsCol::ColumnCount, this);
    m_tableWidget->setHorizontalHeaderLabels({"항목", "전압", "전류"});

    // 그룹헤더 설정
    auto setupHeaderRow = [&](int row, const QString& text) {
        auto* headerItem = new QTableWidgetItem(text);
        headerItem->setBackground(palette().color(QPalette::Window)); // 배경색
        headerItem->setFlags(Qt::ItemIsEnabled);
        m_tableWidget->setItem(row, 0, headerItem);
        m_tableWidget->setSpan(row, 0, 1, MetricsCol::ColumnCount); // 열 병합
    };
    setupHeaderRow(MetricsRow::HeaderApparent, "▼ 피상전력");
    setupHeaderRow(MetricsRow::HeaderReactivePower, "▼ 무효전력");
    setupHeaderRow(MetricsRow::HeaderPowerFactor, "▼ 역률");
    setupHeaderRow(MetricsRow::HeaderThd, "▼ THD");
    setupHeaderRow(MetricsRow::HeaderSymmetrical, "▼ 대칭 성분 분석");
    setupHeaderRow(MetricsRow::HeaderNemaUnbalance, "▼ NEMA Unbalance");
    setupHeaderRow(MetricsRow::HeaederU0U2Unbalance, "▼ U0U2 Unbalance");

    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Title, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Voltage, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Current, QHeaderView::Stretch);

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 각 행의 제목 아이템 설정
    m_tableWidget->setItem(MetricsRow::ResidualRms, MetricsCol::Title, new QTableWidgetItem("잔류 RMS (V/A)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerA, MetricsCol::Title, new QTableWidgetItem("피상전력 (A) (VA)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerB, MetricsCol::Title, new QTableWidgetItem("피상전력 (B) (VA)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerC, MetricsCol::Title, new QTableWidgetItem("피상전력 (C) (VA)"));
    m_tableWidget->setItem(MetricsRow::TotalApparentPower, MetricsCol::Title, new QTableWidgetItem("총 피상전력 (VA)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerA, MetricsCol::Title, new QTableWidgetItem("무효전력 (A) (VAR)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerB, MetricsCol::Title, new QTableWidgetItem("무효전력 (B) (VAR)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerC, MetricsCol::Title, new QTableWidgetItem("무효전력 (C) (VAR)"));
    m_tableWidget->setItem(MetricsRow::TotalReactivePower, MetricsCol::Title, new QTableWidgetItem("총 무효전력 (VAR)"));
    m_tableWidget->setItem(MetricsRow::PowerFactorA, MetricsCol::Title, new QTableWidgetItem("역률 (A)"));
    m_tableWidget->setItem(MetricsRow::PowerFactorB, MetricsCol::Title, new QTableWidgetItem("역률 (B)"));
    m_tableWidget->setItem(MetricsRow::PowerFactorC, MetricsCol::Title, new QTableWidgetItem("역률 (C)"));
    m_tableWidget->setItem(MetricsRow::TotalPowerFactor, MetricsCol::Title, new QTableWidgetItem("총 역률"));
    m_tableWidget->setItem(MetricsRow::ThdA, MetricsCol::Title, new QTableWidgetItem("THD (A) (%)"));
    m_tableWidget->setItem(MetricsRow::ThdB, MetricsCol::Title, new QTableWidgetItem("THD (B) (%)"));
    m_tableWidget->setItem(MetricsRow::ThdC, MetricsCol::Title, new QTableWidgetItem("THD (C) (%)"));
    m_tableWidget->setItem(MetricsRow::SystemThd, MetricsCol::Title, new QTableWidgetItem("Ststem THD (%)"));
    m_tableWidget->setItem(MetricsRow::ZeroSequence, MetricsCol::Title, new QTableWidgetItem("영상분 (V/A)"));
    m_tableWidget->setItem(MetricsRow::PositiveSequence, MetricsCol::Title, new QTableWidgetItem("정상분 (V/A)"));
    m_tableWidget->setItem(MetricsRow::NegativeSequence, MetricsCol::Title, new QTableWidgetItem("역상분 (V/A)"));
    m_tableWidget->setItem(MetricsRow::NemaUnbalance, MetricsCol::Title, new QTableWidgetItem("Unbal"));
    m_tableWidget->setItem(MetricsRow::U0Unbalance, MetricsCol::Title, new QTableWidgetItem("U0 Unbalance (%)"));
    m_tableWidget->setItem(MetricsRow::U2Unbalance, MetricsCol::Title, new QTableWidgetItem("U2 Unbalance (%)"));

    // 초기값 아이템 생성
    for(int row{0}; row < MetricsRow::RowCount; ++row) {
        // 제목 셀 설정
        if(m_tableWidget->item(row, MetricsCol::Title)) {
            m_tableWidget->item(row, MetricsCol::Title)->setFlags(Qt::ItemIsEnabled);
        }
        for(int col{1}; col < MetricsCol::ColumnCount; ++col) {
            if(!m_tableWidget->item(row, col)) {
                auto valueItem = new QTableWidgetItem("0.0");
                valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignCenter);
                valueItem->setFlags(Qt::ItemIsEnabled);
                m_tableWidget->setItem(row, col, valueItem);
            }
        }
    }

    // 형식에 맞지 않는 셀 비워둠
    m_tableWidget->item(MetricsRow::ApparentPowerA, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::ApparentPowerB, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::ApparentPowerC, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::TotalApparentPower, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::ReactivePowerA, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::ReactivePowerB, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::ReactivePowerC, MetricsCol::Current)->setText("");
    m_tableWidget->item(MetricsRow::TotalReactivePower, MetricsCol::Current)->setText("");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tableWidget);
    setLayout(mainLayout);
}

void AdditionalMetricsWindow::updateData(const OneSecondSummaryData& data)
{

    // 잔류 정보 표시
    m_tableWidget->item(MetricsRow::ResidualRms, MetricsCol::Voltage)
        ->setText(QString::number(data.residualVoltageRms, 'f', 3));
    m_tableWidget->item(MetricsRow::ResidualRms, MetricsCol::Current)
        ->setText(QString::number(data.residualCurrentRms, 'f', 3));

    // 피상전력 정보 표시
    m_tableWidget->item(MetricsRow::ApparentPowerA, MetricsCol::Voltage)
        ->setText(QString::number(data.apparentPower.a, 'f', 3));
    m_tableWidget->item(MetricsRow::ApparentPowerB, MetricsCol::Voltage)
        ->setText(QString::number(data.apparentPower.b, 'f', 3));
    m_tableWidget->item(MetricsRow::ApparentPowerC, MetricsCol::Voltage)
        ->setText(QString::number(data.apparentPower.c, 'f', 3));
    m_tableWidget->item(MetricsRow::TotalApparentPower, MetricsCol::Voltage)
        ->setText(QString::number(data.totalApparentPower, 'f', 3));

    // 무효전력 정보 표시
    m_tableWidget->item(MetricsRow::ReactivePowerA, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.a, 'f', 3));
    m_tableWidget->item(MetricsRow::ReactivePowerB, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.b, 'f', 3));
    m_tableWidget->item(MetricsRow::ReactivePowerC, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.c, 'f', 3));
    m_tableWidget->item(MetricsRow::TotalReactivePower, MetricsCol::Voltage)
        ->setText(QString::number(data.totalReactivePower, 'f', 3));

    // 역률 정보 표시
    m_tableWidget->item(MetricsRow::PowerFactorA, MetricsCol::Voltage)->setText(QString::number(data.powerFactor.a, 'f', 3));
    m_tableWidget->item(MetricsRow::PowerFactorB, MetricsCol::Voltage)->setText(QString::number(data.powerFactor.b, 'f', 3));
    m_tableWidget->item(MetricsRow::PowerFactorC, MetricsCol::Voltage)->setText(QString::number(data.powerFactor.c, 'f', 3));
    m_tableWidget->item(MetricsRow::TotalPowerFactor, MetricsCol::Voltage)->setText(QString::number(data.totalPowerFactor, 'f', 3));

    // THD 정보 표시
    m_tableWidget->item(MetricsRow::ThdA, MetricsCol::Voltage)->setText(QString::number(data.voltageThd.a, 'f', 3));
    m_tableWidget->item(MetricsRow::ThdA, MetricsCol::Current)->setText(QString::number(data.currentThd.a, 'f', 3));
    m_tableWidget->item(MetricsRow::ThdB, MetricsCol::Voltage)->setText(QString::number(data.voltageThd.b, 'f', 3));
    m_tableWidget->item(MetricsRow::ThdB, MetricsCol::Current)->setText(QString::number(data.currentThd.b, 'f', 3));
    m_tableWidget->item(MetricsRow::ThdC, MetricsCol::Voltage)->setText(QString::number(data.voltageThd.c, 'f', 3));
    m_tableWidget->item(MetricsRow::ThdC, MetricsCol::Current)->setText(QString::number(data.currentThd.c, 'f', 3));
    m_tableWidget->item(MetricsRow::SystemThd, MetricsCol::Voltage)->setText(QString::number(data.systemVoltageThd, 'f', 3));
    m_tableWidget->item(MetricsRow::SystemThd, MetricsCol::Current)->setText(QString::number(data.systemCurrentThd, 'f', 3));

    // 대칭 정보 표시
    m_tableWidget->item(MetricsRow::ZeroSequence, MetricsCol::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.zero.magnitude, 'f', 3));
    m_tableWidget->item(MetricsRow::ZeroSequence, MetricsCol::Current)->setText(QString::number(data.currentSymmetricalComponents.zero.magnitude, 'f', 3));
    m_tableWidget->item(MetricsRow::PositiveSequence, MetricsCol::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.positive.magnitude, 'f', 3));
    m_tableWidget->item(MetricsRow::PositiveSequence, MetricsCol::Current)->setText(QString::number(data.currentSymmetricalComponents.positive.magnitude, 'f', 3));
    m_tableWidget->item(MetricsRow::NegativeSequence, MetricsCol::Voltage)->setText(QString::number(data.voltageSymmetricalComponents.negative.magnitude, 'f', 3));
    m_tableWidget->item(MetricsRow::NegativeSequence, MetricsCol::Current)->setText(QString::number(data.currentSymmetricalComponents.negative.magnitude, 'f', 3));

    // NEMA 불평형
    m_tableWidget->item(MetricsRow::NemaUnbalance, MetricsCol::Voltage)->setText(QString::number(data.nemaVoltageUnbalance, 'f', 3));
    m_tableWidget->item(MetricsRow::NemaUnbalance, MetricsCol::Current)->setText(QString::number(data.nemaCurrentUnbalance, 'f', 3));

    // U0U2 불평형
    m_tableWidget->item(MetricsRow::U0Unbalance, MetricsCol::Voltage)->setText(QString::number(data.voltageU0Unbalance, 'f', 3));
    m_tableWidget->item(MetricsRow::U0Unbalance, MetricsCol::Current)->setText(QString::number(data.voltageU0Unbalance, 'f', 3));
    m_tableWidget->item(MetricsRow::U2Unbalance, MetricsCol::Voltage)->setText(QString::number(data.voltageU2Unbalance, 'f', 3));
    m_tableWidget->item(MetricsRow::U2Unbalance, MetricsCol::Current)->setText(QString::number(data.voltageU2Unbalance, 'f', 3));
}
