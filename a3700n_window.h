#ifndef A37__N_WINDOW_H
#define A37__N_WINDOW_H

#include <QWidget>
#include <deque>
#include "data_point.h"
#include "measured_data.h"

class QListWidget;
class QStackedWidget;
class QLabel;
class QTabWidget;

class DataPage;
class SimulationEngine;

class A3700N_Window : public QWidget
{
    Q_OBJECT
public:
    explicit A3700N_Window(SimulationEngine* engine, QWidget *parent = nullptr);

public slots:
    void updateSummaryData(const OneSecondSummaryData& data);
    void updateMeasuredData(const std::deque<MeasuredData>& data);
    void updateWaveformData(const std::deque<DataPoint>& data);

signals:
    void summaryDataUpdated(const OneSecondSummaryData& data);
    void measuredDataUpdated(const std::deque<MeasuredData>& data);
    void waveformDataUpdated(const std::deque<DataPoint>& data);

private:
    void setupUi();
    QWidget* createTabPage(const QString& type);
    void createAndAddPage(
        QListWidget* submenu,
        QStackedWidget* stack,
        const QString& submenuName,
        const QString& title,
        const QStringList& rowLabels,
        const QString& unit,
        const std::vector<std::function<double(const OneSecondSummaryData&)>>& extractors);

    QTabWidget* m_mainTabs;
    SimulationEngine* m_engine;

    void createVoltagePage(QListWidget* submenu, QStackedWidget* stack);
    void createCurrentPage(QListWidget* submenu, QStackedWidget* stack);
    void createPowerPage(QListWidget* submenu, QStackedWidget* stack);
    void createAnalysisPage(QListWidget* submenu, QStackedWidget* stack);
};

#endif // A37__N_WINDOW_H
