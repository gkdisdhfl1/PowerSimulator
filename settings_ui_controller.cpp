#include "settings_ui_controller.h"
#include "config.h"
#include "main_view.h"
#include "ui_main_view.h"
#include "settings_dialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>

SettingsUiController::SettingsUiController(MainView* view, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent)
    : QObject(parent)
    ,m_view(view)
    ,m_settingsManager(settingsManager)
    ,m_engine(engine)
    ,m_parent(parent)
{
    initializeSettingsMap();

    m_settingsDialog = std::make_unique<SettingsDialog>(m_parent);
    m_settingsDialog->setController(this);
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
    if(result)
        emit presetListChanged(result.value());
    else {
        // 오류처리
        qWarning() << "Failed to get preset list:" << QString::fromStdString(result.error());
    }
}

void SettingsUiController::onRequestPresetValues(const QString& presetName)
{
    // 직접 DB에서 값을 읽어와야함
    // m_settingsMap의 getter은 현재 UI 상태를 반영
    auto maxSizeResult = m_settingsManager.loadSetting(presetName.toStdString(), "maxDataSize", config::Simulation::DefaultDataSize);
    auto graphWidthResult = m_settingsManager.loadSetting(presetName.toStdString(), "graphWidthSec", config::View::GraphWidth::Default);

    if(maxSizeResult && graphWidthResult) {
        emit presetValuesFetched(*maxSizeResult, *graphWidthResult);
    } else {
        // 오류 처리
        qWarning() << "Failed to fetch preset values for" << presetName;
    }
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
    m_engine->parameters().frequency = value;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onCurrentPhaseChanged(int degrees)
{
    m_engine->parameters().currentPhaseOffsetRadians = utils::degreesToRadians(degrees);
}
void SettingsUiController::onTimeScaleChanged(double value)
{
    m_engine->parameters().timeScale = value;
    m_engine->updateCaptureTimer();
}
void SettingsUiController::onSamplingCyclesChanged(double value)
{
    m_engine->parameters().samplingCycles = value;
    m_engine->recalculateCaptureInterval();
}
void SettingsUiController::onSamplesPerCycleChanged(int value)
{
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

void SettingsUiController::handleSettingsDialog()
{
    m_settingsDialog->exec();
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
            m_engine->recalculateCaptureInterval();
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
            m_engine->updateCaptureTimer();
        },
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [this] { return m_engine->parameters().samplingCycles; },
        [this](const SettingValue& val) {
            double value = std::get<double>(val);
            m_view->getUi()->samplingCyclesControl->setValue(value);
            m_engine->parameters().samplingCycles = value;
            m_engine->recalculateCaptureInterval();
        },
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [this] { return m_engine->parameters().samplesPerCycle;},
        [this](const SettingValue& val) {
            double value = std::get<int>(val);
            m_view->getUi()->samplesPerCycleControl->setValue(value);
            m_engine->recalculateCaptureInterval();
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

std::expected<void, std::string> SettingsUiController::applySettingsToUi(std::string_view presetName)
{
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

            qDebug() << "Applying:" << QString::fromStdString(key)
                     << "| Loaded value: " << QVariant::fromValue(*loadResult)
                     << "| Type:" << typeid(*loadResult).name();

            info.setter(*loadResult); // setter에 T 타입의 값을 담은 variant 전달
            return {};
        }, info.defaultValue);

        if(!result) return result; // 오류 발생 시 즉시 전파
    }
    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
    // m_statusbar->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
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

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
    return {};
}

std::optional<QString> SettingsUiController::promptUserWithPresetList(const QString& title, const QString& label)
{
    auto presetsResult = m_settingsManager.getAllPresetNames();
    if(!presetsResult) {
        QMessageBox::warning(m_parent, "오류", QString::fromStdString(presetsResult.error()));
        return std::nullopt; // optional에 값이 없음을 나타냄
    }

    if(presetsResult.value().empty()) {
        QMessageBox::information(m_parent, "알림", "저장된 설정이 없습니다.");
        return std::nullopt;
    }

    QStringList presetItem;
    for(const auto& name : presetsResult.value()) {
        presetItem << QString::fromStdString(name);
    }

    bool ok;
    QString selectedPreset = QInputDialog::getItem(m_parent, title, label, presetItem, 0, false, &ok);

    if(ok && !selectedPreset.isEmpty()) {
        return selectedPreset; // optional에 선택된 값을 담아 반환
    }

    return std::nullopt; // 사용자가 취소했음을 나타냄
}
