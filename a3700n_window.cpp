#include "a3700n_window.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>

A3700N_Window::A3700N_Window(QWidget *parent)
    : QWidget{parent}
{
    setupUi();
}

void A3700N_Window::setupUi()
{
    // 1. 왼쪽 서브메뉴 생성
    m_submenu = new QListWidget();
    m_submenu->addItems({"RMS", "Fundamental", "THD %", "Frequency", "Residual"});
    m_submenu->setMaximumWidth(150);

    // 2. 오른쪽 컨텐츠 스택 위젯 생성 및 페이지 추가
    m_contentsStack = new QStackedWidget();
    m_contentsStack->addWidget(createTablePage(m_rmsTable, {"A", "B", "C", "Average"}, "V"));
    m_contentsStack->addWidget(createTablePage(m_fundamentalTable, {"A", "B", "C", "Average"}, "V"));
    m_contentsStack->addWidget(createTablePage(m_thdTable, {"A", "B", "C"}, "&"));
    m_contentsStack->addWidget(createTablePage(m_frequencyTable, {"Frequency"}, "Hz"));
    m_contentsStack->addWidget(createTablePage(m_residualTable, {"RMS", "Fund."}, "V"));

    // 3. 전체 레이아웃 설정
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_submenu);
    mainLayout->addWidget(m_contentsStack, 1);

    // 4. 시그널 슬롯 연결
    connect(m_submenu, &QListWidget::currentRowChanged, m_contentsStack, &QStackedWidget::setCurrentIndex);

    m_submenu->setCurrentRow(0); // 기본 선택
}

QWidget* A3700N_Window::createTablePage(QTableWidget*& table, const QStringList& rowLabels, const QString& unit)
{
    table = new QTableWidget(rowLabels.count(), 2); // 행 개수, 2열 (값 ,단위)
    table->setHorizontalHeaderLabels({"Value", "Unit"});
    table->setVerticalHeaderLabels(rowLabels);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setStretchLastSection(QHeaderView::Stretch);

    for(int row = 0; row < rowLabels.count(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem("0.000"));
        table->setItem(row, 1, new QTableWidgetItem(unit));
        table->item(row, 1)->setFlags(Qt::ItemIsEnabled);
    }
    return table;
}

void A3700N_Window::updateData(const OneSecondSummaryData& data)
{
    // RMS
    m_rmsTable->item(0, 0)->setText(QString::number(data.totalVoltageRms.a, 'f', 3));
    m_rmsTable->item(1, 0)->setText(QString::number(data.totalVoltageRms.b, 'f', 3));
    m_rmsTable->item(2, 0)->setText(QString::number(data.totalVoltageRms.c, 'f', 3));
    double rmsAverage = (data.totalVoltageRms.a + data.totalVoltageRms.b + data.totalVoltageRms.c) / 3.0;
    m_rmsTable->item(3, 0)->setText(QString::number(rmsAverage, 'f', 3));

    // Fundamental
    m_fundamentalTable->item(0, 0)->setText(QString::number(data.fundamentalVoltage[0].rms, 'f', 3));
    m_fundamentalTable->item(1, 0)->setText(QString::number(data.fundamentalVoltage[1].rms, 'f', 3));
    m_fundamentalTable->item(2, 0)->setText(QString::number(data.fundamentalVoltage[2].rms, 'f', 3));
    double fundAverage = (data.fundamentalVoltage[0].rms + data.fundamentalVoltage[1].rms + data.fundamentalVoltage[2].rms) / 3.0;
    m_fundamentalTable->item(3, 0)->setText(QString::number(fundAverage, 'f', 3));

    // THD
    m_thdTable->item(0, 0)->setText(QString::number(data.voltageThd.a, 'f', 3));
    m_thdTable->item(1, 0)->setText(QString::number(data.voltageThd.b, 'f', 3));
    m_thdTable->item(2, 0)->setText(QString::number(data.voltageThd.c, 'f', 3));

    // Frequency
    m_frequencyTable->item(0, 0)->setText(QString::number(data.frequency, 'f', 3));

    // Residual
    m_residualTable->item(0, 0)->setText(QString::number(data.residualVoltageRms, 'f', 3));
    // 일단 residual fundamental 값이 없으므로 일단 0표시
    m_residualTable->item(1, 0)->setText(QString::number(0.0, 'f', 3));
}
