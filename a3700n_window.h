#ifndef A37__N_WINDOW_H
#define A37__N_WINDOW_H

#include <QWidget>
#include "measured_data.h"

class QTableWidget;
class QListWidget;
class QStackedWidget;
class QLabel;

class A3700N_Window : public QWidget
{
    Q_OBJECT
public:
    explicit A3700N_Window(QWidget *parent = nullptr);

public slots:
    void updateData(const OneSecondSummaryData& data);

private:
    void setupUi();
    QWidget* createTablePage(QTableWidget*& table, const QStringList& rowLabels, const QString& unit)    ;

    QListWidget* m_submenu;
    QStackedWidget* m_contentsStack;

    // 각 페이지의 UI 요소들을 멤버로 가져서 updateData에서 접근
    QTableWidget* m_rmsTable;
    QTableWidget* m_fundamentalTable;
    QTableWidget* m_thdTable;
    QTableWidget* m_frequencyTable;
    QTableWidget* m_residualTable;
signals:
};

#endif // A37__N_WINDOW_H
