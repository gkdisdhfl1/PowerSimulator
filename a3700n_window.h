#ifndef A37__N_WINDOW_H
#define A37__N_WINDOW_H

#include <QWidget>
#include "measured_data.h"

class QListWidget;
class QStackedWidget;
class QLabel;

class DataPage;

class A3700N_Window : public QWidget
{
    Q_OBJECT
public:
    explicit A3700N_Window(QWidget *parent = nullptr);

public slots:
    void updateData(const OneSecondSummaryData& data);

signals:
    void dataUpdated(const OneSecondSummaryData& data);

private:
    void setupUi();
    void createAndAddPage( const QString& submenuName,
                          const QString& title,
                          const QStringList& rowLabels,
                          const QString& unit,
                          const std::vector<std::function<double(const OneSecondSummaryData&)>>& extractors);

    QListWidget* m_submenu;
    QStackedWidget* m_contentsStack;
};

#endif // A37__N_WINDOW_H
