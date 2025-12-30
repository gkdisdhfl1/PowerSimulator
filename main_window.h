#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "control_panel_state.h"

#include <QMainWindow>

// 전방 선언
class ControlPanel;
class GraphWindow;
// class QAction;
class AnalysisGraphWindow;
class FundamentalAnalysisGraphWindow;
class HarmonicAnalysisGraphWindow;
class PhasorView;
class OneSecondSummaryWindow;
class AdditionalMetricsWindow;
class ThreePhaseDialog;
class PidTuningDialog;
class A3700N_Window;
class DemandCalculator;

class QLabel;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // SystemController를 위한 Getter들
    ControlPanel* getControlPanel() const { return m_controlPanel; }
    GraphWindow* getGraphWindow() const { return m_graphWindow; }
    AnalysisGraphWindow* getAnalysisGraphWindow() const { return m_analysisGraphWindow; }
    PhasorView* getPhasorView() const { return m_phasorView; }
    FundamentalAnalysisGraphWindow* getFundamentalGraphWindow() const { return m_fundamentalAnalysisGraphWindow; }
    HarmonicAnalysisGraphWindow* getHarmonicGraphWindow() const { return m_harmonicAnalysisGraphWindow; }
    OneSecondSummaryWindow* getOneSecondWindow() const { return m_oneSecondSummaryWindow; }
    AdditionalMetricsWindow* getAdditionalMetricsWindow() const { return m_additionalMetricsWindow; }
    DemandCalculator* getDemandCalculator() const { return m_demandCalculator.get(); }
    A3700N_Window* getA3700Window() const { return m_a3700nWindow.get(); }

    // 다이얼로그 접근자
    ThreePhaseDialog* getThreePhaseDialog() const { return m_threePhaseDialog.get(); }
    PidTuningDialog* getPidTuningDialog() const { return m_pidTuningDialog.get(); }

    // 메뉴 액션 접근자
    QAction* getActionSettings() const { return m_actionSettings; }
    QAction* getActionPidTuning() const { return m_actionPidTuning; }
    QAction* getActionThreePhase() const { return m_actionThreePhaseSettings; }
    QAction* getActionA3700() const { return m_actionA3700; }

public slots:
    void onPresetLoaded(const ControlPanelState& state);
    void showThreePhaseDialog();
    void showPidTuningDialog();
    void showA3700Window();

private slots:
    void updatePlaceholderVisibility();
    void updateFpsLabel();

private:
    void createMenus();

    // UI 구성 요소 및 레이아웃을 초기화
    void setupUiComponents();
    void setupConnections();

    // --- UI 컴포넌트 ---
    ControlPanel* m_controlPanel = nullptr;           // 시뮬레이션 제어를 위한 도크 위젯
    GraphWindow* m_graphWindow = nullptr;             // 메인 파형 그래프
    AnalysisGraphWindow* m_analysisGraphWindow = nullptr;
    PhasorView* m_phasorView = nullptr;               // 페이저 다이어그램 뷰
    FundamentalAnalysisGraphWindow *m_fundamentalAnalysisGraphWindow = nullptr;
    HarmonicAnalysisGraphWindow *m_harmonicAnalysisGraphWindow = nullptr;
    OneSecondSummaryWindow* m_oneSecondSummaryWindow = nullptr;
    AdditionalMetricsWindow* m_additionalMetricsWindow = nullptr;

    std::unique_ptr<A3700N_Window> m_a3700nWindow;
    std::unique_ptr<DemandCalculator> m_demandCalculator;
    std::unique_ptr<ThreePhaseDialog> m_threePhaseDialog;
    std::unique_ptr<PidTuningDialog> m_pidTuningDialog;

    // Docking 관련
    QDockWidget* m_controlDock = nullptr;
    QDockWidget* m_placeholderDock = nullptr;

    // 기타
    QLabel* m_fpsLabel;
    QTimer* m_fpsTimer;
    uint64_t m_frameCount;

    // 메뉴 엑션
    QAction* m_actionSettings;
    QAction* m_actionPidTuning;
    QAction* m_actionThreePhaseSettings;
    QAction* m_actionA3700;

    // void createSignalSlotConnections();
};
#endif // MAIN_WINDOW_H
