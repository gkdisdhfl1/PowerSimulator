#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class GraphWindow;
class SettingsDialog;
class SimulationEngine;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_settingButton_clicked();

private:
    Ui::MainWindow *ui;
    GraphWindow *m_graphWindow;
    SettingsDialog *m_settingsDialog;
    SimulationEngine *m_engine;
};
#endif // MAIN_WINDOW_H
