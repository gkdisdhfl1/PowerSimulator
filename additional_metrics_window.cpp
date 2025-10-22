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
    m_tableWidget->setItem(MetricsRow::Thd, MetricsCol::Title, new QTableWidgetItem("THD (%)"));
    m_tableWidget->setItem(MetricsRow::ApparentPower, MetricsCol::Title, new QTableWidgetItem("피상전력 (VA)"));
    m_tableWidget->setItem(MetricsRow::ReactivePower, MetricsCol::Title, new QTableWidgetItem("무효전력 (VAR)"));
    m_tableWidget->setItem(MetricsRow::powerFactor, MetricsCol::Title, new QTableWidgetItem("역률 (%)"));

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
    m_tableWidget->item(MetricsRow::ResidualRms, MetricsCol::Voltage)
        ->setText(QString::number(data.residualVoltageRms, 'f', 3));
    m_tableWidget->item(MetricsRow::ResidualRms, MetricsCol::Current)
        ->setText(QString::number(data.residualCurrentRms, 'f', 3));
}
