#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

// 전방 선언
class SimulationEngine;
class SettingsManager;
class SettingsUiController;
class ControlPanel;
class GraphWindow;
class QAction;
class AnalysisGraphWindow;
class PhasorView;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePlaceholderVisibility();
    void updateFpsLabel();

private:
    QAction* m_actionSettings;
    SimulationEngine *m_engine;

    // View와 Controller들을 소유
    ControlPanel* m_controlPanel;
    QDockWidget* m_controlDock;
    GraphWindow* m_graphWindow;
    AnalysisGraphWindow* m_analysisGraphWindow;
    PhasorView* m_phasorView;
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;
    QDockWidget* m_placeholderDock;
    QLabel* m_fpsLabel;
    QTimer* m_fpsTimer;
    int m_frameCount;

    void createSignalSlotConnections(); // 조립 역할
    void setupUiComponents();
    void createMenus();
};
#endif // MAIN_WINDOW_H
