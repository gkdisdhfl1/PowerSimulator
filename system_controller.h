#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <QObject>
#include <QThread>

class SettingsUiController;
class SettingsManager;
class SimulationEngine;
class MainWindow;

class SystemController : public QObject
{
    Q_OBJECT
public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController();

    void initialize();

private:
    void setupThread();
    void createConnections();

    // Core Components
    QThread m_simulationThread;
    SimulationEngine* m_engine = nullptr; // 스레드로 이동되므로 포인터로 관리

    // UI Components
    std::unique_ptr<MainWindow> m_mainWindow;

    // Controllers, Managers
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsController;
};

#endif // SYSTEM_CONTROLLER_H
