#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <QWidget>
#include <control_panel_state.h>

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
    void onEngineSamplingCyclesChanged(double newFrequency);

private slots:
    void updateCurrentPhaseLabel(int value);

signals:
    // ui control의 이벤트를 외부로 전달하는 시그널들
    void startStopClicked();
    void settingsClicked();
    void autoScrollToggled(bool enabled);
    void trackingToggled(bool enabled); // 자동 추적 토글 시그널

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

    ValueControlWidget* m_voltageHarmonicOrder;
    ValueControlWidget* m_voltageHarmonicMagnitude;
    ValueControlWidget* m_voltageHarmonicPhase;
    ValueControlWidget* m_currentHarmonicOrder;
    ValueControlWidget* m_currentHarmonicMagnitude;
    ValueControlWidget* m_currentHarmonicPhase;

    FineTuningDial* m_currentPhaseDial;
    QLabel* m_currentPhaseLabel;

    QRadioButton* m_perSampleRadioButton;
    QRadioButton* m_perHalfCycleRadioButton;
    QRadioButton* m_perCycleRadioButton;

    QCheckBox* m_autoScrollCheckBox;
    QPushButton* m_trackingButton;
};

#endif // CONTROL_PANEL_H
