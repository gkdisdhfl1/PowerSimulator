#include "settings_ui_controller.h"
#include "config.h"
#include "main_view.h"
#include "ui_main_view.h"
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

SettingsUiController::SettingsUiController(MainView* view, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent)
    : QObject(parent)
    ,m_view(view)
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
        std::expected<SettingValue, std::string> result = std::visit([&](auto&& defaultValue)->std::expected<SettingValue, std::string> { // 반환 타입 명시
            return m_settingsManager.loadSetting(presetStdString, key, defaultValue);
        }, info.defaultValue);

        if(result) {
            QString displayName = m_keyNameMap.value(QString::fromStdString(key));
            if(displayName.isEmpty()) continue;

            // QVariant로 변환하는 작업을 visit 바깥에서 수행
            QVariant displayValue;
            std::visit([&](auto&& value) {
                displayValue = QVariant::fromValue(value);
            }, *result);

            if(QString::fromStdString(key) == "updateMode") {
                int mode = displayValue.toInt();
                // qDebug() << "display value : " << displayValue;
                // qDebug() << "mode : " << mode;
                QString modeStr = (mode == 0) ? "Per Sample" : (mode == 1) ? "Per Half Cycle" :
                                                               "Per Cycle";
                // qDebug() << "modeStr : " << modeStr;
                previewData[displayName] = modeStr;
            } else {
                previewData[displayName] = displayValue;
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
    int currentMaxSize = std::get<int>(m_settingsMap.at("maxDataSize").getter());
    double currentGrapWidth = std::get<double>(m_settingsMap.at("graphWidthSec").getter());
    emit currentSettingsFetched(currentMaxSize, currentGrapWidth);
}

void SettingsUiController::onApplyDialogSettings(int maxDataSize, double graphWidth)
{
    m_settingsMap.at("maxDataSize").setter(maxDataSize);
    m_settingsMap.at("graphWidthSec").setter(graphWidth);
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
    // 어떤 버튼을 눌렀는지 확인하여 모드 변경
    if(m_view->getUi()->perSampleRadioButton->isChecked()) {
        m_engine->parameters().updateMode = SimulationEngine::UpdateMode::PerSample;
    } else if(m_view->getUi()->perHalfCycleRadioButton->isChecked()) {
        m_engine->parameters().updateMode = SimulationEngine::UpdateMode::PerHalfCycle;
    } else if(m_view->getUi()->PerCycleRadioButton->isChecked()) {
        m_engine->parameters().updateMode = SimulationEngine::UpdateMode::PerCycle;
    }
}

void SettingsUiController::showSettingsDialog()
{
    int currentMaxSize = std::get<int>(m_settingsMap.at("maxDataSize").getter());
    double currentGraphWidth = std::get<double>(m_settingsMap.at("graphWidthSec").getter());

    if (m_settingsDialog->openWithValues(currentMaxSize, currentGraphWidth) == QDialog::Accepted) {
        // Dialog가 닫혔을 때, 이유를 확인
        if(m_settingsDialog->getResultState() == SettingsDialog::DialogResult::Accepted) {
            int newMaxSize = m_settingsDialog->getMaxSize(); // Dialog에 getter 필요
            double newGraphWidth = m_settingsDialog->getGraphWidth(); // Dialog에 getter 필요

            m_settingsMap.at("maxDataSize").setter(newMaxSize);
            m_settingsMap.at("graphWidthSec").setter(newGraphWidth);
        }
    }
}

void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    m_settingsMap["voltageAmplitude"]   = {
        [this] { return m_engine->parameters().amplitude;},
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->voltageControlWidget->setValue(value);
            m_engine->parameters().amplitude = value;
        },
        config::Source::Amplitude::Default
    };
    m_settingsMap["currentAmplitude"]  = {
        [this] { return m_engine->parameters().currentAmplitude; },
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->currentAmplitudeControl->setValue(value);
            m_engine->parameters().currentAmplitude = value;
        },
        config::Source::Current::DefaultAmplitude
    };
    m_settingsMap["frequency"] = {
        [this] { return m_engine->parameters().frequency; },
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->frequencyControlWidget->setValue(value);
            m_engine->parameters().frequency = value;
            // m_engine->recalculateCaptureInterval();
        },
        config::Source::Frequency::Default
    };
    m_settingsMap["currentPhaseOffset"] = {
        [this] { return m_view->getUi()->currentPhaseDial->value(); },
        [this](const SettingValue& val){
            int degrees = std::get<int>(val);
            m_view->getUi()->currentPhaseDial->setValue(degrees);
            m_engine->parameters().currentPhaseOffsetRadians = utils::degreesToRadians(degrees);
        },
        config::Source::Current::DefaultPhaseOffset
    };
    m_settingsMap["timeScale"] = {
        [this] { return m_engine->parameters().timeScale; },
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->timeScaleWidget->setValue(value);
            m_engine->parameters().timeScale = value;
            // m_engine->updateCaptureTimer();
        },
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [this] { return m_engine->parameters().samplingCycles; },
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->samplingCyclesControl->setValue(value);
            m_engine->parameters().samplingCycles = value;
            // m_engine->recalculateCaptureInterval();
        },
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [this] { return m_engine->parameters().samplesPerCycle;},
        [this](const SettingValue& val) {
            double value = std::get<int>(val);
            m_view->getUi()->samplesPerCycleControl->setValue(value);
            // m_engine->recalculateCaptureInterval();
        },
        config::Sampling::DefaultSamplesPerCycle
    };
    m_settingsMap["maxDataSize"] = {
        [this] { return m_engine->parameters().maxDataSize; },
        [this](const SettingValue& val) {
            m_engine->parameters().maxDataSize = std::get<int>(val);
        },
        config::Simulation::DefaultDataSize
    };
    m_settingsMap["graphWidthSec"] = {
        [this] { return m_view->getUi()->graphViewPlaceholder->getGraphWidth();},
        [this](const SettingValue& val) { m_view->getUi()->graphViewPlaceholder->setGraphWidth(std::get<double>(val));},
        config::View::GraphWidth::Default
    };

    m_settingsMap["updateMode"] = {
        [this]() -> SettingValue {
            return static_cast<int>(m_engine->parameters().updateMode);
        },
        [this](const SettingValue& val) {
            int mode = std::get<int>(val);
            m_engine->parameters().updateMode = static_cast<SimulationEngine::UpdateMode>(mode);
            if(mode == 1) m_view->getUi()->perHalfCycleRadioButton->setChecked(true);
            else if(mode == 2) m_view->getUi()->PerCycleRadioButton->setChecked(true);
            else m_view->getUi()->perSampleRadioButton->setChecked(true);
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

    // 맵을 순회하면 DB에서 값을 불러와 각 setter에 적용
    for(auto const& [key, info] : m_settingsMap) {
        auto result = std::visit([&](auto&& defaultValue)->std::expected<void, std::string> {
            // defaultValue의 실제 타입을 추론
            using T = std::decay_t<decltype(defaultValue)>;

            // DB에서 실제 타입에 맞는 기본값을 사용하여 설정을 불러옴
            auto loadResult=  m_settingsManager.loadSetting(presetName, key, defaultValue);
            if(!loadResult) {
                return std::unexpected(loadResult.error());
            }

            // qDebug() << "Applying:" << QString::fromStdString(key)
            //          << "| Loaded value: " << QVariant::fromValue(*loadResult)
            //          << "| Type:" << typeid(*loadResult).name();

            info.setter(*loadResult); // setter에 T 타입의 값을 담은 variant 전달
            return {};
        }, info.defaultValue);

        if(!result) {
            m_blockUiSignals = false;
            return result; // 오류 발생 시 즉시 전파
        }
    }
    m_blockUiSignals = false;

    // 모든 값이 변경 된 후, 일괄 적용
    m_engine->recalculateCaptureInterval();

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data() , presetName.size())), StatusBarTimeOut);
    return {};
}

std::expected<void, std::string> SettingsUiController::saveUiToSettings(std::string_view presetName)
{
    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        auto result = std::visit([&](auto&& value) {
            return m_settingsManager.saveSetting(presetName, key, value);
        }, info.getter());

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), StatusBarTimeOut);
    return {};
}
