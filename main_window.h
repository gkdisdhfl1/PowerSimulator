#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class SimulationEngine;
class SettingsManager;
class SettingsUiController;
class ControlPanel;
class GraphWindow;

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
    SimulationEngine *m_engine;

    // View와 Controller들을 소유
    ControlPanel* m_controlPanel;
    GraphWindow* m_graphWindow;
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;

    void createSignalSlotConnections(); // 조립 역할
    void setupUiComponents();
};
#endif // MAIN_WINDOW_H
