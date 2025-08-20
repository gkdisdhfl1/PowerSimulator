#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class SimulationEngine;
class SettingsManager;
class SettingsUiController;
class MainView;

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

private:
    Ui::MainWindow *ui;
    MainView *m_view;
    SimulationEngine *m_engine;

    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;

    void createSignalSlotConnections(); // 조립 역할
    void createActions(); // 메뉴바
};
#endif // MAIN_WINDOW_H
