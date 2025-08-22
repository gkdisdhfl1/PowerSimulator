#include "settings_ui_controller.h"
#include "control_panel.h"
#include "config.h"
#include "settings_dialog.h"
#include "settings_manager.h"
#include "simulation_engine.h"
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
constexpr const char* GraphWidth        = "그래프 시간 폭";
constexpr const char* UpdateMode        = "갱신 모드";
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
}

// --- slot 구현 ---
void SettingsUiController::onSaveAsPresetRequested(const QString& presetName)
{
    auto result = saveUiToSettings(presetName.toStdString());
    emit taskFinished(result, "프리셋이 성공적으로 저장되었습니다.");
}
void SettingsUiController::onLoadPresetRequested(const QString& presetName)
{
    auto result = applySettingsToUi(presetName.toStdString());
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
        qDebug() << "result value: " << result.value();
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
        // std::expected<SettingValue, std::string> result = std::visit([&](auto&& defaultValue)->std::expected<SettingValue, std::string> { // 반환 타입 명시
        //     return m_settingsManager.loadSetting(presetStdString, key, defaultValue);
        // }, info.defaultValue);

        // if(result) {
        //     QString displayName = m_keyNameMap.value(QString::fromStdString(key));
        //     if(displayName.isEmpty()) continue;

        //     // QVariant로 변환하는 작업을 visit 바깥에서 수행
        //     QVariant displayValue;
        //     std::visit([&](auto&& value) {
        //         displayValue = QVariant::fromValue(value);
        //     }, *result);

        //     if(QString::fromStdString(key) == "updateMode") {
        //         int mode = displayValue.toInt();
        //         // qDebug() << "display value : " << displayValue;
        //         // qDebug() << "mode : " << mode;
        //         QString modeStr = (mode == 0) ? "Per Sample" : (mode == 1) ? "Per Half Cycle" :
        //                                                        "Per Cycle";
        //         // qDebug() << "modeStr : " << modeStr;
        //         previewData[displayName] = modeStr;
        //     } else {
        //         previewData[displayName] = displayValue;
        //     }
        // } else {
        //     qWarning() << "Could not load setting" << QString::fromStdString(key)
        //                << "for preset" << presetName;
        // }


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

void SettingsUiController::onRequestCurrentSettings()
{
    int currentMaxSize = m_engine->parameters().maxDataSize;
    double currentGrapWidth = m_engine->parameters().graphWidthSec;
    emit currentSettingsFetched(currentMaxSize, currentGrapWidth);
}

void SettingsUiController::onApplyDialogSettings(int maxDataSize, double graphWidth)
{
    m_engine->parameters().maxDataSize = maxDataSize;
    m_engine->parameters().graphWidthSec = graphWidth;
}

void SettingsUiController::onAmplitudeChanged(double value)
{
    m_engine->parameters().amplitude = value;
}
void SettingsUiController::onCurrentAmplitudeChanged(double value)
{
    m_engine->parameters().currentAmplitude = value;
}
void SettingsUiController::onFrequencyChanged(double value)
{
    if(m_blockUiSignals) return;
    m_engine->parameters().frequency = value;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onCurrentPhaseChanged(int degrees)
{
    m_engine->parameters().currentPhaseOffsetRadians = utils::degreesToRadians(degrees);
}
void SettingsUiController::onTimeScaleChanged(double value)
{
    if(m_blockUiSignals) return;
    m_engine->parameters().timeScale = value;
    m_engine->updateCaptureTimer();
}
void SettingsUiController::onSamplingCyclesChanged(double value)
{
    if(m_blockUiSignals) return;
    m_engine->parameters().samplingCycles = value;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onSamplesPerCycleChanged(int value)
{
    if(m_blockUiSignals) return;
    m_engine->parameters().samplesPerCycle = value;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onUpdateModeChanged()
{
    ControlPanelState state = m_controlPanel->getState();

    // state 객체에 담긴 updateMode 값을 엔진의 파라미터에 직접 설정
    m_engine->parameters().updateMode = static_cast<SimulationEngine::UpdateMode>(state.updateMode);
}

void SettingsUiController::showSettingsDialog()
{
    int currentMaxSize = m_engine->parameters().maxDataSize;
    double currentGraphWidth = m_engine->parameters().graphWidthSec;

    if (m_settingsDialog->openWithValues(currentMaxSize, currentGraphWidth) == QDialog::Accepted) {
        // Dialog가 닫혔을 때, 이유를 확인
        if(m_settingsDialog->getResultState() == SettingsDialog::DialogResult::Accepted) {
            int newMaxSize = m_settingsDialog->getMaxSize(); // Dialog에 getter 필요
            double newGraphWidth = m_settingsDialog->getGraphWidth(); // Dialog에 getter 필요

            m_engine->parameters().maxDataSize = newMaxSize;
            m_engine->parameters().graphWidthSec = newGraphWidth;
        }
    }
}

void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    m_settingsMap["voltageAmplitude"] = {
        [](const ControlPanelState& s){return s.amplitude; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.amplitude = std::get<double>(val);
        },
        config::Source::Amplitude::Default
    };
    m_settingsMap["currentAmplitude"]  = {
        [](const ControlPanelState& s) { return s.currentAmplitude; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.currentAmplitude = std::get<double>(val);
        },
        config::Source::Current::DefaultAmplitude
    };
    m_settingsMap["frequency"] = {
        [](const ControlPanelState& s) { return s.frequency; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.frequency = std::get<double>(val);
        },
        config::Source::Frequency::Default
    };
    m_settingsMap["currentPhaseOffset"] = {
        [](const ControlPanelState& s) { return s.currentPhaseDegrees; },
        [](ControlPanelState& s, const SettingValue& val){
            s.currentPhaseDegrees = std::get<int>(val);
        },
        config::Source::Current::DefaultPhaseOffset
    };
    m_settingsMap["timeScale"] = {
        [](const ControlPanelState& s) { return s.timeScale; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.timeScale = std::get<double>(val);
        },
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [](const ControlPanelState& s) { return s.samplingCycles; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.samplingCycles = std::get<double>(val);
        },
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [](const ControlPanelState& s) { return s.samplesPerCycle; },
        [](ControlPanelState& s, const SettingValue& val) {
            s.samplesPerCycle = std::get<int>(val);
        },
        config::Sampling::DefaultSamplesPerCycle
    };
    // m_settingsMap["maxDataSize"] = {
    //     [this] { return m_engine->parameters().maxDataSize; },
    //     [this](const SettingValue& val) {
    //         m_engine->parameters().maxDataSize = std::get<int>(val);
    //     },
    //     config::Simulation::DefaultDataSize
    // };
    // m_settingsMap["graphWidthSec"] = {
    //     [this] { return m_view->getUi()->graphViewPlaceholder->getGraphWidth();},
    //     [this](const SettingValue& val) { m_view->getUi()->graphViewPlaceholder->setGraphWidth(std::get<double>(val));},
    //     config::View::GraphWidth::Default
    // };

    m_settingsMap["updateMode"] = {
        [](const ControlPanelState& s) {
            return s.updateMode;
        },
        [](ControlPanelState& s, const SettingValue& val) {
            s.updateMode = std::get<int>(val);
        },
        0
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
    m_keyNameMap["graphWidthSec"] = PresetKeys::GraphWidth;
    m_keyNameMap["updateMode"] = PresetKeys::UpdateMode;
}

std::expected<void, std::string> SettingsUiController::applySettingsToUi(std::string_view presetName)
{
    m_blockUiSignals = true; // 프리셋 적용 시 슬롯 호출 잠시 무시
    ControlPanelState state = m_controlPanel->getState(); // 현재 상태를 기본값으로

    // 맵을 순회하면 DB에서 값을 불러와 각 setter에 적용
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
        info.setter(state, *loadedValueResult);
    }

    // 완성된 state로 UI를 한 번에 업데이트
    m_controlPanel->setState(state);

    // UI와 별개인 엔진 파라미터들도 업데이트
    if(auto res = m_settingsManager.loadSetting(presetName, "maxDataSize", m_engine->parameters().maxDataSize); res) {
        m_engine->parameters().maxDataSize = *res;
    }
    if(auto res = m_settingsManager.loadSetting(presetName, "graphWidthSec", m_engine->parameters().graphWidthSec); res) {
        m_engine->parameters().graphWidthSec = *res;
    }

    m_blockUiSignals = false;
    m_engine->recalculateCaptureInterval(); // 엔진 파라미터 재계산
    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data() , presetName.size())), StatusBarTimeOut);
    return {};
}

std::expected<void, std::string> SettingsUiController::saveUiToSettings(std::string_view presetName)
{
    ControlPanelState state = m_controlPanel->getState(); // 현재 UI 상태를 가져옴

    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        auto valueToSave = info.getter(state);
        auto result = std::visit([&](auto&& value) {
            return m_settingsManager.saveSetting(presetName, key, value);
        }, valueToSave);

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    // UI와 별개인 엔진 파라미터들도 저장
    if(auto res = m_settingsManager.saveSetting(presetName, "maxDataSize", m_engine->parameters().maxDataSize); !res) return res;
    if(auto res = m_settingsManager.saveSetting(presetName, "graphWidthSec", m_engine->parameters().graphWidthSec); !res) return res;

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), StatusBarTimeOut);
    return {};
}
