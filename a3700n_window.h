#ifndef A37__N_WINDOW_H
#define A37__N_WINDOW_H

#include <QWidget>
#include "measured_data.h"

class QListWidget;
class QStackedWidget;
class QLabel;
class QTabWidget;

class DataPage;

class A3700N_Window : public QWidget
{
    Q_OBJECT
public:
    explicit A3700N_Window(QWidget *parent = nullptr);

public slots:
    void updateData(const OneSecondSummaryData& data);

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
    std::vector<DataPage*> m_pages;

    void createVoltagePage(QListWidget* submenu, QStackedWidget* stack);
    void createCurrentPage(QListWidget* submenu, QStackedWidget* stack);
    void createPowerPage(QListWidget* submenu, QStackedWidget* stack);
};

#endif // A37__N_WINDOW_H
