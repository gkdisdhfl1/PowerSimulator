#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class SettingsDialog;
class SimulationEngine;
class ValueControlWidget;

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
    void handleSettingButtonClicked();
    void onEngineRuninngStateChanged(bool isRunning);

private:
    Ui::MainWindow *ui;
    SettingsDialog *m_settingsDialog;
    SimulationEngine *m_engine;

    void setupUiWidgets();
    void createSignalSlotConnections();
};
#endif // MAIN_WINDOW_H
