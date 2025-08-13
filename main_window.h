#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class SimulationEngine;
class SettingsManager;
class SettingsUiController;

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
    void onEngineRuninngStateChanged(bool isRunning);
    void onActionSettings();

private:
    Ui::MainWindow *ui;
    SimulationEngine *m_engine;

    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;

    void setupUiWidgets();
    void createSignalSlotConnections();
};
#endif // MAIN_WINDOW_H
