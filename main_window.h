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
class AdditionalMetricsWindow;
class ThreePhaseDialog;
class PidTuningDialog;
class A3700N_Window;
class DemandCalculator;

// MainWindow 클래스
// UI의 중앙 컨트롤러 역할을 하는 메인 애플리케이션 창입니다.
// 다음 요소들을 초기화하고 관리합니다:
// - SimulationEngine (로직)
// - ControlPanel (사용자 입력)
// - 다양한 GraphWindow 및 분석 뷰 (시각화)
// - 대화 상자 및 설정
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // MainWindow 생성자
    // engine: SimulationEngine 인스턴스에 대한 포인터
    explicit MainWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePlaceholderVisibility();
    void updateFpsLabel();
    void onPresetLoaded();
    void showThreePhaseDialog();
    void showPidTuningDialog();
    void showA3700Window();

private:
    QAction* m_actionSettings;
    QAction* m_actionPidTuning;
    QAction* m_actionThreePhaseSettings;
    QAction* m_actionA3700;

    SimulationEngine *m_engine; // 시뮬레이션 엔진에 대한 참조

    // --- UI 구성 요소 ---
    ControlPanel* m_controlPanel;           // 시뮬레이션 제어를 위한 도크 위젯
    PhasorView* m_phasorView;               // 페이저 다이어그램 뷰
    GraphWindow* m_graphWindow;             // 메인 파형 그래프
    FundamentalAnalysisGraphWindow *m_fundamentalAnalysisGraphWindow;
    HarmonicAnalysisGraphWindow *m_harmonicAnalysisGraphWindow;
    OneSecondSummaryWindow* m_oneSecondSummaryWindow;
    AdditionalMetricsWindow* m_additionalMetricsWindow;

    QDockWidget* m_controlDock;
    AnalysisGraphWindow* m_analysisGraphWindow;
    QDockWidget* m_placeholderDock;
    QLabel* m_fpsLabel;
    QTimer* m_fpsTimer;
    int m_frameCount;

    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<SettingsUiController> m_settingsUiController;
    std::unique_ptr<A3700N_Window> m_a3700nWindow;
    std::unique_ptr<DemandCalculator> m_demandCalculator;

    // --- 대화 상자 (Dialogs) ---
    std::unique_ptr<ThreePhaseDialog> m_threePhaseDialog;
    std::unique_ptr<PidTuningDialog> m_pidTuningDialog;

    // 구성 요소 간의 신호와 슬롯을 연결합니다.
    // 엔진, 제어 패널, 그래프 간의 통신 흐름을 설정합니다.
    void createSignalSlotConnections(); 

    // UI 구성 요소 및 레이아웃을 초기화합니다.
    void setupUiComponents();
    
    void createMenus();
};
#endif // MAIN_WINDOW_H
