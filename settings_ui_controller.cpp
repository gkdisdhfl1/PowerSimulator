 #include "settings_ui_controller.h"
#include "control_panel.h"
#include "config.h"
#include "pid_tuning_dialog.h"
#include "settings_dialog.h"
#include "settings_manager.h"
#include "simulation_engine.h"
#include "three_phase_dialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>

namespace PresetKeys {
constexpr const char* Amplitude         = "진폭 (Voltage)";
constexpr const char* CurrentAmplitude  = "진폭 (Current)";
constexpr const char* Frequency         = "주파수";
constexpr const char* CurrentPhase      = "위상차";
constexpr const char* TimeScale         = "시간 배율";
constexpr const char* SamplingCycles    = "초당 cycle";
constexpr const char* SamplesPerCycle   = "cycle당 sample";
constexpr const char* MaxDataSize       = "데이터 최대 개수";
// constexpr const char* GraphWidth        = "그래프 시간 폭";
// constexpr const char* UpdateMode        = "갱신 모드";

// 고조파 설정 키
constexpr const char* VoltageHarmonicOrder      = "전압 고조파 차수";
constexpr const char* VoltageHarmonicMagnitude  = "전압 고조파 크기";
constexpr const char* VoltageHarmonicPhase      = "전압 고조파 위상";
constexpr const char* CurrentHarmonicOrder      = "전류 고조파 차수";
constexpr const char* CurrentHarmonicMagnitude  = "전류 고조파 크기";
constexpr const char* CurrentHarmonicPhase      = "전류 고조파 위상";

// 3상 설정 키
constexpr const char* VoltageBAmplitude     = "B상 전압 크기";
constexpr const char* VoltageBPhase         = "B상 전압 위상";
constexpr const char* VoltageCAmplitude     = "C상 전압 크기";
constexpr const char* VoltageCPhase         = "C상 전압 위상";
constexpr const char* CurrentBAmplitude     = "B상 전류 크기";
constexpr const char* CurrentBPhase         = "B상 전류 위상";
constexpr const char* CurrentCAmplitude     = "C상 전류 크기";
constexpr const char* CurrentCPhase         = "C상 전류 위상";
}

namespace {
constexpr int StatusBarTimeOut = 3000;
}

SettingsUiController::SettingsUiController(ControlPanel* controlPanel, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent)
    : QObject(parent)
    ,m_controlPanel(controlPanel)
    ,m_settingsManager(settingsManager)
    ,m_engine(engine)
    ,m_parent(parent)
{
    initializeSettingsMap();
    initializeKeyNameMap();

    m_settingsDialog = std::make_unique<SettingsDialog>(this, m_parent);
    m_pidTuningDialog = std::make_unique<PidTuningDialog>(m_parent);

    connect(m_pidTuningDialog.get(), &PidTuningDialog::settingsApplied, this, &SettingsUiController::onCoefficientsChanged);
    // connect(m_threePhaseDialog, &ThreePhaseDialog::valueChanged, this, &SettingsUiController::onThreePhaseValueChanged);
}

// --- public slot 구현 ---
void SettingsUiController::onSaveAsPresetRequested(const QString& presetName)
{
    auto result = saveEngineToSettings(presetName.toStdString());
    emit taskFinished(result, "프리셋이 성공적으로 저장되었습니다.");
}
void SettingsUiController::onLoadPresetRequested(const QString& presetName)
{
    auto result = applySettingsToEngine(presetName.toStdString());
    // qDebug() << "[Debug] SettingsUiController: Emitting presetLoaded signal.";
    emit taskFinished(result, "프리셋을 성공적으로 적용했습니다.");
}
void SettingsUiController::onDeletePresetRequested(const QString& presetName)
{
    auto result = m_settingsManager.deletePreset(presetName.toStdString());
    emit taskFinished(result, "프리셋이 성공적으로 삭제되었습니다.");
}
void SettingsUiController::onRenamePresetRequested(const QString& oldName, const QString& newName)
{
    auto result = m_settingsManager.renamePreset(oldName.toStdString(), newName.toStdString());
    emit taskFinished(result, "프리셋 이름이 변경되었습니다.");
}

void SettingsUiController::onRequestPresetList()
{
    auto result = m_settingsManager.getAllPresetNames();
    if(result) {
        // qDebug() << "result value: " << result.value();
        emit presetListChanged(result.value());
    }
    else {
        // 오류처리
        qWarning() << "Failed to get preset list:" << QString::fromStdString(result.error());
    }
}

void SettingsUiController::onRequestPresetValues(const QString& presetName)
{
    QVariantMap previewData;
    std::string presetStdString = presetName.toStdString();

    // m_settingsMap을 순회하며 모든 키에 대해 DB값을 읽음
    for(const auto& [key, info] : m_settingsMap) {
        QVariant loadedValueAsVariant = std::visit([&](auto&& defaultValue) -> QVariant {
            // DB에서 설정을 불러옴
            auto loadResult = m_settingsManager.loadSetting(presetStdString, key, defaultValue);

            if(loadResult) {
                // 성공 시, 결과값을 QVariant로 변환하여 반환
                return QVariant::fromValue(*loadResult);
            }
            // 실패 시, 유효하지 않은 QVariant 반환
            return QVariant();
        }, info.defaultValue);

        // QVariant가 유효할 때에만 맵에 추가
        if(loadedValueAsVariant.isValid()) {
            QString displayName = m_keyNameMap.value(QString::fromStdString(key));
            if(displayName.isEmpty()) continue;

            // updateMode의 경우, 숫자 대신 문자열로 반환하여 표시
            if(QString::fromStdString(key) == "updateMode") {
                int mode = loadedValueAsVariant.toInt();
                QString modeStr = (mode == 0) ? "Per Sample" : (mode == 1) ? "Per Half Cycle" : "Per Cycle";
                previewData[displayName] = modeStr;
            } else {
                previewData[displayName] = loadedValueAsVariant;
            }
        } else {
            qWarning() << "Could not load setting" << QString::fromStdString(key)
                           << "for preset" << presetName;
        }
    }

    emit presetValuesFetched(previewData);
}

void SettingsUiController::onApplyDialogSettings(const SimulationEngine* params)
{
    if(!m_engine) return;

    m_engine->m_maxDataSize.setValue(params->m_maxDataSize.value());
    m_engine->m_graphWidthSec.setValue(params->m_graphWidthSec.value());
    m_engine->m_voltage_B_amplitude.setValue(params->m_voltage_B_amplitude.value());
    m_engine->m_voltage_B_phase_deg.setValue(params->m_voltage_B_phase_deg.value());
    m_engine->m_voltage_C_amplitude.setValue(params->m_voltage_C_amplitude.value());
    m_engine->m_voltage_C_phase_deg.setValue(params->m_voltage_C_phase_deg.value());
    m_engine->m_current_B_amplitude.setValue(params->m_current_B_amplitude.value());
    m_engine->m_current_B_phase_deg.setValue(params->m_current_B_phase_deg.value());
    m_engine->m_current_C_amplitude.setValue(params->m_current_C_amplitude.value());
    m_engine->m_current_C_phase_deg.setValue(params->m_current_C_phase_deg.value());
}

void SettingsUiController::onAmplitudeChanged(double value)
{
    m_engine->m_amplitude.setValue(value);
}
void SettingsUiController::onCurrentAmplitudeChanged(double value)
{
    m_engine->m_currentAmplitude.setValue(value);
}
void SettingsUiController::onFrequencyChanged(double value)
{
    m_engine->m_frequency.setValue(value);
    if(m_blockUiSignals) return;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onCurrentPhaseChanged(int degrees)
{
    m_engine->m_currentPhaseOffsetRadians.setValue(utils::degreesToRadians(degrees));
}
void SettingsUiController::onTimeScaleChanged(double value)
{
    m_engine->m_timeScale.setValue(value);
    if(m_blockUiSignals) return;
    m_engine->updateCaptureTimer();
}
void SettingsUiController::onSamplingCyclesChanged(double value)
{
    m_engine->m_samplingCycles.setValue(value);
    if(m_blockUiSignals) return;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onSamplesPerCycleChanged(int value)
{
    m_engine->m_samplesPerCycle.setValue(value);
    if(m_blockUiSignals) return;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onUpdateModeChanged()
{
    ControlPanelState state = m_controlPanel->getState();

    // state 객체에 담긴 updateMode 값을 엔진의 파라미터에 직접 설정
    m_engine->m_updateMode.setValue(static_cast<UpdateMode>(state.updateMode));
}

void SettingsUiController::showSettingsDialog()
{
    // SimulationEngine* params = m_engine;

    if (m_settingsDialog->exec() == QDialog::Accepted) {
        // // Dialog가 닫혔을 때, 이유를 확인
        // if(m_settingsDialog->getResultState() == SettingsDialog::DialogResult::Accepted) {
        //     int newMaxSize = m_settingsDialog->getMaxSize();
        //     double newGraphWidth = m_settingsDialog->getGraphWidth();

        //     requestMaxSizeChange(newMaxSize);
        //     m_engine->m_params.graphWidthSec = newGraphWidth;
        // }
    }
}

void SettingsUiController::onTrackingToggled(bool enabled)
{
    m_engine->enableFrequencyTracking(enabled);
}

void SettingsUiController::showPidTuningDialog()
{
    // FrequencyTracker로부터 현재 PID 계수를 가져옴
    auto fllCoeffs = m_engine->getFrequencyTracker()->getFllCoefficients();
    auto zcCoeffs = m_engine->getFrequencyTracker()->getZcCoefficients();

    // 다이얼로그 현재 값 설정
    m_pidTuningDialog->setInitialValues(fllCoeffs, zcCoeffs);

    m_pidTuningDialog->show();
    m_pidTuningDialog->raise();
    m_pidTuningDialog->activateWindow();
}

void SettingsUiController::onCoefficientsChanged(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs)
{
    m_engine->getFrequencyTracker()->setFllCoefficients(fllCoeffs);
    m_engine->getFrequencyTracker()->setZcCoefficients(zcCoeffs);

    qDebug() << "PID 계수 설정 완료";
    qDebug() << "pllCoeffs : " << fllCoeffs.Kd << ", " << fllCoeffs.Ki << ", " << fllCoeffs.Kd;
    qDebug() << "zclCoeffs : " << zcCoeffs.Kd << ", " << zcCoeffs.Ki << ", " << zcCoeffs.Kd;
}

void SettingsUiController::onHarmonicsChanged()
{
    const auto state = m_controlPanel->getState();
    auto& params = m_engine;

    params->m_voltageHarmonic.setValue(state.voltageHarmonic);
    params->m_currentHarmonic.setValue(state.currentHarmonic);
}

void SettingsUiController::onThreePhaseValueChanged(int type, double value)
{
    auto& params = m_engine;

    switch(static_cast<ThreePhaseDialog::ParamType>(type))
    {
    case ThreePhaseDialog::VoltageBAmplitude:
        if(params->m_voltage_B_amplitude.value() != value) params->m_voltage_B_amplitude.setValue(value);
        break;
    case ThreePhaseDialog::VoltageBPhase:
        if(params->m_voltage_B_phase_deg.value() != value) params->m_voltage_B_phase_deg.setValue(value);
        break;
    case ThreePhaseDialog::VoltageCAmplitude:
        if(params->m_voltage_C_amplitude.value() != value) params->m_voltage_C_amplitude.setValue(value);
        break;
    case ThreePhaseDialog::VoltageCPhase:
        if(params->m_voltage_C_phase_deg.value() != value) params->m_voltage_C_phase_deg.setValue(value);
        break;
    case ThreePhaseDialog::CurrentBAmplitude:
        if(params->m_current_B_amplitude.value() != value) params->m_current_B_amplitude.setValue(value);
        break;
    case ThreePhaseDialog::CurrentBPhase:
        if(params->m_current_B_phase_deg.value() != value) params->m_current_B_phase_deg.setValue(value);
        break;
    case ThreePhaseDialog::CurrentCAmplitude:
        if(params->m_current_C_amplitude.value() != value) params->m_current_C_amplitude.setValue(value);
        break;
    case ThreePhaseDialog::CurrentCPhase:
        if(params->m_current_C_phase_deg.value() != value) params->m_current_C_phase_deg.setValue(value);
        break;
    default:
        break;
    }
}

// void SettingsUiController::showThreePhaseDialog()
// {
//     const auto& currentParams = m_engine;
//     m_threePhaseDialog->setInitialValues(currentParams);

//     m_threePhaseDialog->show();
//     m_threePhaseDialog->raise();
//     m_threePhaseDialog->activateWindow();
// }
// -------------------------

// ----- private 헬퍼 함수들 ------
void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    // getter: m_engine의 Property에서 값을 가져옴
    // setter: m_engine의 Property에서 값을 설정함

    m_settingsMap["voltageAmplitude"] = {
        [&](){return m_engine->m_amplitude.value(); },
        [&](const SettingValue& val) {
            m_engine->m_amplitude.setValue(std::get<double>(val));
        },
        config::Source::Amplitude::Default
    };
    m_settingsMap["currentAmplitude"] = {
        [&](){return m_engine->m_currentAmplitude.value(); },
        [&](const SettingValue& val) {
            m_engine->m_currentAmplitude.setValue(std::get<double>(val));
        },
        config::Source::Current::DefaultAmplitude
    };
    m_settingsMap["frequency"] = {
        [&](){ return m_engine->m_frequency.value(); },
        [&](const SettingValue& val) {
            m_engine->m_frequency.setValue(std::get<double>(val));
        },
        config::Source::Frequency::Default
    };
    m_settingsMap["currentPhaseOffset"] = {
                                           [&]() { return static_cast<int>(utils::radiansToDegrees(m_engine->m_currentPhaseOffsetRadians.value())); },
        [&](const SettingValue& val){
            m_engine->m_currentPhaseOffsetRadians.setValue(std::get<int>(val));
        },
        config::Source::Current::DefaultPhaseOffset
    };
    m_settingsMap["timeScale"] = {
        [&]() { return m_engine->m_timeScale.value(); },
        [&](const SettingValue& val) {
            m_engine->m_timeScale.setValue(std::get<double>(val));
        },
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [&]() { return m_engine->m_samplingCycles.value(); },
        [&](const SettingValue& val) {
            m_engine->m_samplingCycles.setValue(std::get<double>(val));
        },
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [&]() { return m_engine->m_samplesPerCycle.value(); },
        [&](const SettingValue& val) {
            m_engine->m_samplesPerCycle.setValue(std::get<int>(val));
        },
        config::Sampling::DefaultSamplesPerCycle
    };
    m_settingsMap["updateMode"] = {
        [&]() { return static_cast<int>(m_engine->m_updateMode.value()); },
        [&](const SettingValue& val) {
            m_engine->m_updateMode.setValue(static_cast<UpdateMode>(std::get<int>(val)));
        },
        0 // PerSample
    };

    // 고조파 관련 설정
    m_settingsMap["voltHarmonicOrder"] = {
        [&]() {
            return m_engine->m_voltageHarmonic.value().order;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_voltageHarmonic.value();
            hc.order = std::get<int>(val);
            m_engine->m_voltageHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultOrder
    };
    m_settingsMap["voltHarmonicMagnitude"] = {
        [&]() {
            return m_engine->m_voltageHarmonic.value().magnitude;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_voltageHarmonic.value();
            hc.magnitude = std::get<double>(val);
            m_engine->m_voltageHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultMagnitude
    };
    m_settingsMap["voltHarmonicPhase"] = {
        [&]() {
            return m_engine->m_voltageHarmonic.value().phase;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_voltageHarmonic.value();
            hc.phase = std::get<double>(val);
            m_engine->m_voltageHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultPhase
    };
    m_settingsMap["currHarmonicOrder"] = {
        [&]() {
            return m_engine->m_currentHarmonic.value().order;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_currentHarmonic.value();
            hc.order = std::get<int>(val);
            m_engine->m_currentHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultOrder
    };
    m_settingsMap["currHarmonicMagnitude"] = {
        [&]() {
            return m_engine->m_currentHarmonic.value().magnitude;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_currentHarmonic.value();
            hc.magnitude = std::get<double>(val);
            m_engine->m_currentHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultMagnitude
    };
    m_settingsMap["currHarmonicPhase"] = {
        [&]() {
            return m_engine->m_currentHarmonic.value().phase;
        },
        [&](const SettingValue& val) {
            auto hc = m_engine->m_currentHarmonic.value();
            hc.phase = std::get<double>(val);
            m_engine->m_currentHarmonic.setValue(hc);
        },
        config::Harmonics::DefaultPhase
    };


    // 3상 관련 설정
    m_settingsMap["voltageBAmplitude"] = {
        [&]() {
            return m_engine->m_voltage_B_amplitude.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_voltage_B_amplitude.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultAmplitudeB
    };
    m_settingsMap["voltageBPhase"] = {
        [&]() {
            return m_engine->m_voltage_B_phase_deg.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_voltage_B_phase_deg.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultPhaseB_deg
    };
    m_settingsMap["voltageCAmplitude"] = {
        [&]() {
            return m_engine->m_voltage_C_amplitude.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_voltage_C_amplitude.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultAmplitudeC
    };
    m_settingsMap["voltageCPhase"] = {
        [&]() {
            return m_engine->m_voltage_C_phase_deg.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_voltage_C_phase_deg.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultPhaseC_deg
    };
    m_settingsMap["currentBAmplitude"] = {
        [&]() {
            return m_engine->m_current_B_amplitude.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_current_B_amplitude.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultCurrentAmplitudeB
    };
    m_settingsMap["currentBPhase"] = {
        [&]() {
            return m_engine->m_current_B_phase_deg.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_current_B_phase_deg.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultCurrentPhaseB_deg
    };
    m_settingsMap["currentCAmplitude"] = {
        [&]() {
            return m_engine->m_current_C_amplitude.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_current_C_amplitude.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultCurrentAmplitudeC
    };
    m_settingsMap["currentCPhase"] = {
        [&]() {
            return m_engine->m_current_C_phase_deg.value();
        },
        [&](const SettingValue& val) {
            m_engine->m_current_C_phase_deg.setValue(std::get<double>(val));
        },
        config::Source::ThreePhase::DefaultCurrentPhaseC_deg
    };
}

void SettingsUiController::initializeKeyNameMap()
{
    m_keyNameMap["voltageAmplitude"] = PresetKeys::Amplitude;
    m_keyNameMap["currentAmplitude"] = PresetKeys::CurrentAmplitude;
    m_keyNameMap["frequency"] = PresetKeys::Frequency;
    m_keyNameMap["currentPhaseOffset"] = PresetKeys::CurrentPhase;
    m_keyNameMap["timeScale"] = PresetKeys::TimeScale;
    m_keyNameMap["samplingCycles"] = PresetKeys::SamplingCycles;
    m_keyNameMap["samplesPerCycle"] = PresetKeys::SamplesPerCycle;
    m_keyNameMap["maxDataSize"] = PresetKeys::MaxDataSize;
    // m_keyNameMap["graphWidthSec"] = PresetKeys::GraphWidth;
    // m_keyNameMap["updateMode"] = PresetKeys::UpdateMode;

    // 고조파 관련 설정
    m_keyNameMap["voltHarmonicOrder"] = PresetKeys::VoltageHarmonicOrder;
    m_keyNameMap["voltHarmonicMagnitude"] = PresetKeys::VoltageHarmonicMagnitude;
    m_keyNameMap["voltHarmonicPhase"] = PresetKeys::VoltageHarmonicPhase;
    m_keyNameMap["currHarmonicOrder"] = PresetKeys::CurrentHarmonicOrder;
    m_keyNameMap["currHarmonicMagnitude"] = PresetKeys::CurrentHarmonicMagnitude;
    m_keyNameMap["currHarmonicPhase"] = PresetKeys::CurrentHarmonicPhase;

    // 3상 관련 설정
    m_keyNameMap["voltageBAmplitude"] = PresetKeys::VoltageBAmplitude;
    m_keyNameMap["voltageBPhase"] = PresetKeys::VoltageBPhase;
    m_keyNameMap["voltageCAmplitude"] = PresetKeys::VoltageCAmplitude;
    m_keyNameMap["voltageCPhase"] = PresetKeys::VoltageCPhase;
    m_keyNameMap["currentBAmplitude"] = PresetKeys::CurrentBAmplitude;
    m_keyNameMap["currentBPhase"] = PresetKeys::CurrentBPhase;
    m_keyNameMap["currentCAmplitude"] = PresetKeys::CurrentCAmplitude;
    m_keyNameMap["currentCPhase"] = PresetKeys::CurrentCPhase;
}

void SettingsUiController::requestMaxSizeChange(int newSize)
{
    const int currentDataSize = m_engine->getDataSize();
    bool applyChange = true;

    // 새 최대 크기가 현재 데이터 개수보다 작을 경우
    if(newSize < currentDataSize) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(m_parent, "데이터 축소 경고",
                                      QString("데이터 최대 개수를 %1개에서 %2개로 줄이면 이전 데이터 일부가 영구적으로 삭제됩니다. \n\n계속하시겠습니까?").arg(currentDataSize).arg(newSize),
                                      QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::No)
            applyChange = false;
    }

    if(applyChange) {
        emit maxDataSizeChangeRequested(newSize);
    }

}

std::expected<void, std::string> SettingsUiController::applySettingsToEngine(std::string_view presetName)
{
    m_blockUiSignals = true; // 프리셋 적용 시 슬롯 호출 잠시 무시

    // 맵을 순회하면 DB에서 값을 불러와 각 Property에 적용
    for(auto const& [key, info] : m_settingsMap) {
        std::expected<SettingValue, std::string> loadedValueResult = std::visit([&](auto&& defaultValue) -> std::expected<SettingValue, std::string> {
            auto loadResult = m_settingsManager.loadSetting(presetName, key, defaultValue);

            if(loadResult) {
                // 성공 시, 결과값을 variant에 담아 반환
                return *loadResult;
            } else {
                // 실패 시, 에러를 그대로 반환
                return std::unexpected(loadResult.error());
            }
        }, info.defaultValue);

        if(!loadedValueResult) {
            m_blockUiSignals = false;
            return std::unexpected(loadedValueResult.error());
        }
        info.setter(*loadedValueResult);
    }

    // UI와 별개인 엔진 파라미터들도 업데이트
    if(auto res = m_settingsManager.loadSetting(presetName, "maxDataSize", m_engine->m_maxDataSize.value()); res) {
        requestMaxSizeChange(*res);
    }
    if(auto res = m_settingsManager.loadSetting(presetName, "graphWidthSec", m_engine->m_graphWidthSec.value()); res) {
        m_engine->m_graphWidthSec.setValue(*res);
    }

    m_blockUiSignals = false;
    m_engine->recalculateCaptureInterval(); // 엔진 파라미터 재계산
    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data() , presetName.size())), StatusBarTimeOut);
    return {};
}

std::expected<void, std::string> SettingsUiController::saveEngineToSettings(std::string_view presetName)
{

    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        auto valueToSave = info.getter();
        auto result = std::visit([&](auto&& value) {
            return m_settingsManager.saveSetting(presetName, key, value);
        }, valueToSave);

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    // UI와 별개인 엔진 파라미터들도 저장
    if(auto res = m_settingsManager.saveSetting(presetName, "maxDataSize", m_engine->m_maxDataSize.value()); !res) return res;
    if(auto res = m_settingsManager.saveSetting(presetName, "graphWidthSec", m_engine->m_graphWidthSec.value()); !res) return res;

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), StatusBarTimeOut);
    return {};
}
