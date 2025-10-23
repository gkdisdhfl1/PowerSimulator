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
    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Title, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Voltage, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(MetricsCol::Current, QHeaderView::Stretch);

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 각 행의 제목 아이템 설정
    m_tableWidget->setItem(MetricsRow::ResidualRms, MetricsCol::Title, new QTableWidgetItem("잔류 RMS (V/A)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerA, MetricsCol::Title, new QTableWidgetItem("피상전력A (VA)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerB, MetricsCol::Title, new QTableWidgetItem("피상전력B (VA)"));
    m_tableWidget->setItem(MetricsRow::ApparentPowerC, MetricsCol::Title, new QTableWidgetItem("피상전력C (VA)"));
    m_tableWidget->setItem(MetricsRow::TotalApparentPower, MetricsCol::Title, new QTableWidgetItem("총 피상전력 (VA)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerA, MetricsCol::Title, new QTableWidgetItem("무효전력A (VAR)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerB, MetricsCol::Title, new QTableWidgetItem("무효전력B (VAR)"));
    m_tableWidget->setItem(MetricsRow::ReactivePowerC, MetricsCol::Title, new QTableWidgetItem("무효전력C (VAR)"));
    m_tableWidget->setItem(MetricsRow::TotalReactivePower, MetricsCol::Title, new QTableWidgetItem("총 무효전력 (VAR)"));

    // 초기값 아이템 생성
    for(int row{0}; row < MetricsRow::RowCount; ++row) {
        for(int col{1}; col < MetricsCol::ColumnCount; ++col) {
            m_tableWidget->setItem(row, col, new QTableWidgetItem("0.0"));
        }
    }

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tableWidget);
    setLayout(mainLayout);
}

void AdditionalMetricsWindow::updateData(const AdditionalMetricsData& data)
{
    double totalApparentPower = data.apparentPower.a + data.apparentPower.b + data.apparentPower.c;
    double totalReactivePower = data.reactivePower.a + data.reactivePower.b + data.reactivePower.c;

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
        ->setText(QString::number(totalApparentPower, 'f', 3));

    // 무효전력 정보 표시
    m_tableWidget->item(MetricsRow::ReactivePowerA, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.a, 'f', 3));
    m_tableWidget->item(MetricsRow::ReactivePowerB, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.b, 'f', 3));
    m_tableWidget->item(MetricsRow::ReactivePowerC, MetricsCol::Voltage)
        ->setText(QString::number(data.reactivePower.c, 'f', 3));
    m_tableWidget->item(MetricsRow::TotalReactivePower, MetricsCol::Voltage)
        ->setText(QString::number(totalReactivePower, 'f', 3));
}
