#ifndef A37__N_WINDOW_H
#define A37__N_WINDOW_H

#include <QWidget>
#include "demand_data.h"
#include "measured_data.h"

class QListWidget;
class QStackedWidget;
class QLabel;
class QTabWidget;

class DataPage;
class DataSource;
class SimulationEngine;

class A3700N_Window : public QWidget
{
    Q_OBJECT
public:
    explicit A3700N_Window(QWidget *parent = nullptr);

public slots:
    void updateSummaryData(const OneSecondSummaryData& data);
    void updateDemandData(const DemandData& data);

signals:
    void summaryDataUpdated(const OneSecondSummaryData& data);
    void demandDataUpdated(const DemandData& data);

private:
    void setupUi();
    QWidget* createTabPage(const QString& type);
    void createAndAddPage(
        QListWidget* submenu,
        QStackedWidget* stack,
        const QString& submenuName,
        const QString& title,
        const QString& unit,
        const std::vector<DataSource>& dataSources);

    QTabWidget* m_mainTabs;

    void createVoltagePage(QListWidget* submenu, QStackedWidget* stack);
    void createCurrentPage(QListWidget* submenu, QStackedWidget* stack);
    void createPowerPage(QListWidget* submenu, QStackedWidget* stack);
    void createAnalysisPage(QListWidget* submenu, QStackedWidget* stack);

    struct PageConfig {
        QString submenuName;
        QString title;
        QString unit;
        std::vector<DataSource> dataSources;
    };
};

#endif // A37__N_WINDOW_H
