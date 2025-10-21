 #ifndef SETTINGS_UI_CONTROLLER_H
#define SETTINGS_UI_CONTROLLER_H

#include <QVariantMap>
#include <expected>
#include "control_panel_state.h"
#include "frequency_tracker.h"
#include "simulation_engine.h"

class SettingsDialog;
class PidTuningDialog;
class ControlPanel;
class SettingsManager;
class QDoubleSpinBox;
class ThreePhaseDialog;

class SettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsUiController(ControlPanel* controlPanel, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent);


signals:
    void taskFinished(const std::expected<void, std::string>& result, const QString& successMessage);
    void presetListChanged(const std::vector<std::string>& presetList);;
    void presetValuesFetched(const QVariantMap& data);
    void currentSettingsFetched(SimulationEngine::Parameters& params);
    void maxDataSizeChangeRequested(int maxSize);

public slots:
    // View(SettingsDialog)로부터 오는 요청을 처리하는 슬롯
    void onSaveAsPresetRequested(const QString& presetName);
    void onLoadPresetRequested(const QString& presetName);
    void onDeletePresetRequested(const QString& presetName);
    void onRenamePresetRequested(const QString& oldName, const QString& newName);

    // SettingsDialog가 프리셋 목록이나 상세 값을 요청할 때 호출될 슬롯
    void onRequestPresetList();
    void onRequestPresetValues(const QString& presetName);
    void onRequestCurrentSettings();
    void onApplyDialogSettings(const SimulationEngine::Parameters& params);
    void showSettingsDialog();

    // ControlPanel의 실시간 변경에 반응하는 슬롯들
    void onAmplitudeChanged(double value);
    void onCurrentAmplitudeChanged(double value);
    void onFrequencyChanged(double value);
    void onCurrentPhaseChanged(int degrees);
    void onTimeScaleChanged(double value);
    void onSamplingCyclesChanged(double value);
    void onSamplesPerCycleChanged(int value);
    void onUpdateModeChanged(); // 라디오 버튼은 매개변수 필요 없음
    void onTrackingToggled(bool enabled);

    // PID 튜닝 다이얼로그 관련 슬롯
    void showPidTuningDialog();
    void onCoefficientsChanged(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs);

    void onHarmonicsChanged();

    // 3상 변경 관련 슬롯
    void showThreePhaseDialog();
    void onThreePhaseValueChanged(int type, double value);
private:
    using SettingValue = std::variant<int, double>;
    using StateGetter = std::function<SettingValue(const ControlPanelState&)>;
    using StateSetter = std::function<void(ControlPanelState&, const SettingValue&)>;
    using ParamDoublePtr = double SimulationEngine::Parameters::*;

    struct SettingInfo {
        StateGetter getter;
        StateSetter setter;
        SettingValue defaultValue; // DB에 값이 없을 때 사용할 기본값
    };

    ControlPanel* m_controlPanel;
    SettingsManager& m_settingsManager;
    SimulationEngine* m_engine;
    QWidget* m_parent;        
    std::unique_ptr<SettingsDialog> m_settingsDialog; // SettingsDialog 소유권 이전
    std::unique_ptr<PidTuningDialog> m_pidTuningDialog;
    std::unique_ptr<ThreePhaseDialog> m_threePhaseDialog;

    std::unordered_map<std::string, SettingInfo> m_settingsMap;
    QMap<QString, QString> m_keyNameMap;

    // 헬퍼 함수들
    void initializeSettingsMap();
    void initializeKeyNameMap();
    void requestMaxSizeChange(int newSize);
    std::expected<void, std::string> applySettingsToUi(std::string_view presetName); // 특정 프리셋을 UI에 적용하는 함수
    std::expected<void, std::string> saveUiToSettings(std::string_view presetName); // 현재 UI 상태를 특정 프리셋으로 저장하는 함수

    bool m_blockUiSignals = false;
};

#endif // SETTINGS_UI_CONTROLLER_H
