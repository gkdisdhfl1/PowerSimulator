 #ifndef SETTINGS_UI_CONTROLLER_H
#define SETTINGS_UI_CONTROLLER_H

#include <QVariantMap>
#include <expected>
#include "control_panel_state.h"
#include "frequency_tracker.h"
#include "harmonics_dialog.h"
#include "settings_dialog.h"

class ControlPanel;
class SettingsManager;
class SimulationEngine;

class SettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsUiController(ControlPanel* controlPanel, SettingsManager& settingsManager, QWidget* parent);

    const ControlPanelState& getState() const;

signals:
    void taskFinished(const std::expected<void, std::string>& result, const QString& successMessage);
    void presetListChanged(const std::vector<std::string>& presetList);;
    void presetValuesFetched(const QVariantMap& data);
    void requestDataLossConfirmation(const int currentDataSize, int newSize, bool* ok);
    // void maxDataSizeChangeRequested(int maxSize);
    void presetApplied();

    // 엔진 컨트롤 시그널
    // -----------------------------
    void setAmplitude(double value);
    void setCurrentAmplitude(double value);
    void setFrequency(double value);
    void setCurrentPhase(double value);
    void setTimeScale(double value);
    void setSamplingCycles(double value);
    void setSamplesPerCycle(int value);
    void setUpdateMode(UpdateMode mode);

    // harmonics
    void setVoltageHarmonics(const HarmonicList& list);
    void setCurrentHarmonics(const HarmonicList& list);

    // three phase
    void setVoltageBAmplitude(double value);
    void setVoltageBPhase(double value);
    void setVoltageCAmplitude(double value);
    void setVoltageCPhase(double value);
    void setCurrentBAmplitude(double value);
    void setCurrentBPhase(double value);
    void setCurrentCAmplitude(double value);
    void setCurrentCPhase(double value);

    // 기타
    void requestCaptureIntervalUpdate();
    void setMaxDataSize(int size);
    void setGraphWidth(double width);
    void enableTracking(bool enabled);

    // FrequencyTracker 접근 시그널
    void setFrequencyTrackerCoefficients(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs);

public slots:
    // View(SettingsDialog)로부터 오는 요청을 처리하는 슬롯
    void onSaveAsPresetRequested(const QString& presetName);
    void onLoadPresetRequested(const QString& presetName);
    void onDeletePresetRequested(const QString& presetName);
    void onRenamePresetRequested(const QString& oldName, const QString& newName);
    void showSettingsDialog();

    // SettingsDialog가 프리셋 목록이나 상세 값을 요청할 때 호출될 슬롯
    void onRequestPresetList();
    void onRequestPresetValues(const QString& presetName);
    void onApplyDialogSettings(const int maxDatasize, const int graphWidth);

    // ControlPanel의 실시간 변경에 반응하는 슬롯들
    void onAmplitudeChanged(double value);
    void onCurrentAmplitudeChanged(double value);
    void onFrequencyChanged(double value);
    void onCurrentPhaseChanged(int degrees);
    void onTimeScaleChanged(double value);
    void onSamplingCyclesChanged(double value);
    void onSamplesPerCycleChanged(int value);
    void onUpdateModeChanged(UpdateMode mode);
    void onTrackingToggled(bool enabled);

    // PID 튜닝 다이얼로그 관련 슬롯
    void onCoefficientsChanged(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs);

    void onHarmonicsSettingsRequested();

    // 3상 변경 관련 슬롯
    void onThreePhaseValueChanged(int type, double value);
private:
    struct SettingInfo {
        std::function<QVariant()> getter;
        std::function<void(const QVariant&)> setter;
        QVariant defaultValue; // DB에 값이 없을 때 사용할 기본값
        QString displayName; // UI에 표시될 이름
        bool isAngle = false; // 각도 값인지 여부
    };

    QWidget* m_parent;
    ControlPanelState m_state;
    ControlPanel* m_controlPanel;
    SettingsManager& m_settingsManager;

    std::unique_ptr<SettingsDialog> m_settingsDialog; // SettingsDialog 소유권 이전
    std::unique_ptr<HarmonicsDialog> m_harmonicsDialog;

    std::unordered_map<std::string, SettingInfo> m_settingsMap;

    // 헬퍼 함수들
    void initializeSettingsMap();
    void initializeControlPanelDefaultValues();
    void initializeConnections();
    bool requestMaxSizeChange(int newSize);
    std::expected<void, std::string> applySettingsToEngine(std::string_view presetName); // 특정 프리셋을 UI에 적용하는 함수
    std::expected<void, std::string> saveEngineToSettings(std::string_view presetName); // 현재 UI 상태를 특정 프리셋으로 저장하는 함수

    bool m_blockUiSignals = false;

    // 일반적인 값 바인딩
    template<typename T, typename SignalType>
    void bind(const std::string& key, T& stateVariable, SignalType signal, const QVariant& def, const QString& name)
    {
        m_settingsMap[key] = {
            // Getter: 참조된 state 변수를 바로 반환
            [&stateVariable]() { return QVariant::fromValue(stateVariable); },

            // Setter: 값 갱신 후 시그널 발생
            [this, &stateVariable, signal](const QVariant& v) {
                T val = v.value<T>();
                // 값이 다를 때만 업데이트
                if(stateVariable != val) {
                    stateVariable = val;
                    emit(this->*signal)(val); // 멤버 함수 포인터로 시그널 호출
                }
            },
            def, name
        };
    }

    // 각도 처리용 바인딩 (State: Degree <-> Engine: Radian)
    template <typename SignalType>
    void bindAngle(const std::string& key, double& stateVariable, SignalType signal, const QVariant& def, const QString& name)
    {
        m_settingsMap[key] = {
            [&stateVariable]() { return QVariant::fromValue(stateVariable); },

            [this, &stateVariable, signal](const QVariant& v) {
                double degrees = v.toDouble();
                if(!qFuzzyCompare(stateVariable, degrees)) {
                    stateVariable = degrees;
                    // 엔진으로는 Radian 변환해서 전송
                    emit(this->*signal)(utils::degreesToRadians(degrees));
                }
            },
            def, name
        };
    }

    // Enum 처리용 바인딩 (int <-> Enum)
    template <typename EnumType, typename SignalType>
    void bindEnum(const std::string& key, EnumType& stateVariable, SignalType signal, int def, const QString& name)
    {
        m_settingsMap[key] = {
            [&stateVariable]() { return static_cast<int>(stateVariable);},

            [this, &stateVariable, signal](const QVariant& v) {
                auto mode = static_cast<EnumType>(v.toInt());
                if(stateVariable != mode) {
                    stateVariable - mode;
                    emit (this->*signal)(mode);
                }
            },
            def, name
        };
    }
};

#endif // SETTINGS_UI_CONTROLLER_H
