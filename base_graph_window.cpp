#include "base_graph_window.h"
#include "custom_chart_view.h"
#include <QChart>
#include <QValueAxis>
#include <QGridLayout>

BaseGraphWindow::BaseGraphWindow(QWidget *parent)
    :QWidget(parent)
    , m_chart(std::make_unique<QChart>())
    , m_axisX(new QValueAxis(this))
    , m_chartView(new CustomChartView(m_chart.get(), this))
{
    setupBaseChart();
}

void BaseGraphWindow::setupBaseChart()
{
    m_chartView->setRenderHint(QPainter::Antialiasing);

    // 레이아웃 설정
    auto mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_chartView);

    // 차트 기본 설정
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    // X축 설정
    m_axisX->setLabelFormat(tr("%.1f s"));
    m_axisX->setTitleText(tr("시간 (s)"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
}
