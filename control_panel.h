#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <QWidget>
#include <control_panel_state.h>
#include "collapsible_groupbox.h"

// 클래스 전방 선언
class QPushButton;
class QCheckBox;
class ValueControlWidget;
class FineTuningDial;
class QRadioButton;
class QLabel;
class QGroupBox;
class QFormLayout;
class QVBoxLayout;
class QGridLayout;
class QTabWidget;

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);

    // Controller가 호출할 인터페이스
    ControlPanelState getState() const;
    void setState(const ControlPanelState& state);

public slots:
    // MainWindow 또는 다른 컨트롤러가 이 위젯의 상태를 변경할 때 사용
    void setRunningState(bool isRunning);
    void setAutoScroll(bool enabled);
    // void onEngineSamplingCyclesChanged(double newFrequency);

    // --- Model -> View 바인딩을 위한 슬롯들 ----
    void setAmplitude(double value);
    void setCurrentAmplitude(double value);
    void setFrequency(double value);
    void setCurrentPhase(int degrees);
    void setTimeScale(double value);
    void setSamplingCycles(double value);
    void setSamplesPerCycle(int value);
    void setUpdateMode(UpdateMode mode);
    void setVoltageHarmonic(const HarmonicComponent& hc);
    void setCurrentHarmonic(const HarmonicComponent& hc);

private slots:
    void updateCurrentPhaseLabel(int value);
    void updateVoltageHarmonicPhaseLabel(int value);
    void updateCurrentHarmonicPhaseLabel(int value);

signals:
    // ui control의 이벤트를 외부로 전달하는 시그널들
    void startStopClicked();
    void settingsClicked();
    void autoScrollToggled(bool enabled);
    void trackingToggled(bool enabled); // 자동 추적 토글 시그널
    void waveformVisibilityChanged(int type, bool isVisible);
    void analysisWaveformVisibilityChanged(int type, bool isVisible);
    void phasorVisibilityChanged(int type, bool isVisible);
    void stateLoaded();

    // 파라미터 변경 시그널
    void amplitudeChanged(double value);
    void currentAmplitudeChanged(double value);
    void frequencyChanged(double value);
    void currentPhaseChanged(int degrees);
    void timeScaleChanged(double value);
    void samplingCyclesChanged(double value);
    void samplesPerCycleChanged(int value);
    void updateModeChanged();
    void harmonicChanged();

private:
    // UI 생성 및 초기화를 위한 헬퍼 함수들
    void setupUi();
    void initializeUiValues();
    void createConnections();

    // UI 요소들을 멤버 변수로 소유
    QPushButton* m_startStopButton;
    QPushButton* m_settingButton;

    ValueControlWidget* m_voltageControlWidget;
    ValueControlWidget* m_currentAmplitudeControlWidget;
    ValueControlWidget* m_frequencyControlWidget;
    ValueControlWidget* m_timeScaleControlWidget;
    ValueControlWidget* m_samplingCyclesControlWidget;
    ValueControlWidget* m_samplesPerCycleControlWidget;

    // 고조파 UI
    QTabWidget* m_harmonicsTabWidget;
    ValueControlWidget* m_voltageHarmonicOrder;
    ValueControlWidget* m_voltageHarmonicMagnitude;
    FineTuningDial* m_voltageHarmonicPhaseDial;
    QLabel* m_voltageHarmonicPhaseLabel;
    ValueControlWidget* m_currentHarmonicOrder;
    ValueControlWidget* m_currentHarmonicMagnitude;
    FineTuningDial* m_currentHarmonicPhaseDial;
    QLabel* m_currentHarmonicPhaseLabel;

    FineTuningDial* m_currentPhaseDial;
    QLabel* m_currentPhaseLabel;

    QRadioButton* m_perSampleRadioButton;
    QRadioButton* m_perHalfCycleRadioButton;
    QRadioButton* m_perCycleRadioButton;

    QCheckBox* m_autoScrollCheckBox;
    QPushButton* m_trackingButton;

    CollapsibleGroupBox* m_waveformSelectionGroup;
    std::array<QCheckBox*, 3> m_voltageCheckBox;
    std::array<QCheckBox*, 3> m_currentCheckBox;

    CollapsibleGroupBox* m_analysisSelectionGroup;
    std::array<QCheckBox*, 3> m_rmsVoltageCheckBox;
    std::array<QCheckBox*, 3> m_rmsCurrentCheckBox;
    std::array<QCheckBox*, 3> m_activePowerCheckBox;

    // Phasor 그래프 체크박스
    CollapsibleGroupBox* m_phasorSelectionGroup;
    std::array<QCheckBox*, 3> m_phasorFundVoltageCheckBox;
    std::array<QCheckBox*, 3> m_phasorFundCurrentCheckBox;
    QCheckBox* m_phasorHarmVoltageCheckBox;
    QCheckBox* m_phasorHarmCurrentCheckBox;

};

#endif // CONTROL_PANEL_H
