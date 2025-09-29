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
class FundamentalAnalysisGraphWindow;
class HarmonicAnalysisGraphWindow;
class PhasorView;
class QLabel;
class QTimer;
class OneSecondSummaryWindow;

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
    QAction* m_actionPidTuning;
    SimulationEngine *m_engine;

    // View와 Controller들을 소유
    ControlPanel* m_controlPanel;
    QDockWidget* m_controlDock;
    GraphWindow* m_graphWindow;
    AnalysisGraphWindow* m_analysisGraphWindow;
    FundamentalAnalysisGraphWindow *m_fundamentalAnalysisGraphWindow;
    HarmonicAnalysisGraphWindow *m_harmonicAnalysisGraphWindow;
    PhasorView* m_phasorView;
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;
    QDockWidget* m_placeholderDock;
    QLabel* m_fpsLabel;
    QTimer* m_fpsTimer;
    int m_frameCount;
    OneSecondSummaryWindow* m_oneSecondSummaryWindow;

    void createSignalSlotConnections(); // 조립 역할
    void setupUiComponents();
    void createMenus();
};
#endif // MAIN_WINDOW_H
