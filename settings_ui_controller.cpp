#include "settings_ui_controller.h"
#include "config.h"
#include <QInputDialog>
#include <QMessageBox>

SettingsUiController::SettingsUiController(Ui::MainWindow* ui, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent)
    : QObject(parent)
    ,m_ui(ui)
    ,m_settingsManager(settingsManager)
    ,m_engine(engine)
    ,m_parent(parent)
{
    initializeSettingsMap();
}

void SettingsUiController::handleSaveAction()
{
    bool ok;
    // 사용자에게 프리셋 이름을 입력받는 다이얼로그를 띄움
    QString presetName = QInputDialog::getText(m_parent, "설정 저장", "저장할 설정의 이름을 입력하세요:", QLineEdit::Normal, "", &ok);

    if(ok && !presetName.isEmpty()) {
        auto result = saveUiToSettings(presetName.toStdString());
        if(!result)
            QMessageBox::warning(m_parent, "저장 실패", QString::fromStdString(result.error()));
    }
}

void SettingsUiController::handleLoadAction()
{
    if(auto selectedPreset = promptUserWithPresetList("설정 불러오기", "불러올 설정을 선택하세요:")) {
        auto result = applySettingsToUi(selectedPreset->toStdString());
        if(!result)
            QMessageBox::warning(m_parent, "불러오기 실패", QString::fromStdString(result.error()));
    }
}

void SettingsUiController::handleDeleteAction()
{
    if(auto selectedPreset = promptUserWithPresetList("설정 삭제", "삭제할 설정을 선택하세요:")) {

        // 확인 메세지를 한번 더 보여줌
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(m_parent, "삭제 확인", QString("정말로 '%1' 설정을 삭제하겠습니까?").arg(*selectedPreset), QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes) {
            auto result = m_settingsManager.deletePreset(selectedPreset->toStdString());
            if(result) {
                m_ui->statusbar->showMessage(QString("'%1' 설정을 삭제했습니다.").arg(*selectedPreset), 3000);
            } else {
                QMessageBox::warning(m_parent, "삭제 실패: ", QString::fromStdString(result.error()));
            }
        }
    }
}

void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    m_settingsMap["voltageAmplitude"]   = {
        [this] {return m_ui->voltageControlWidget->value();},
        [this](const SettingValue& val) {m_ui->voltageControlWidget->setValue(std::get<double>(val));},
        config::Source::Amplitude::Default
    };
    m_settingsMap["currentAmplitude"]  = {
        [this] { return m_ui->currentAmplitudeControl->value();},
        [this](const SettingValue& val) {m_ui->currentAmplitudeControl->setValue(std::get<double>(val));},
        config::Source::Current::DefaultAmplitude
    };
    m_settingsMap["frequency"] = {
        [this] { return m_ui->frequencyControlWidget->value();},
        [this](const SettingValue& val) {m_ui->frequencyControlWidget->setValue(std::get<double>(val));},
        config::Source::Frequency::Default
    };
    m_settingsMap["currentPhaseOffset"] = {
        [this] { return m_ui->currentPhaseDial->value();},
        [this](const SettingValue& val){m_ui->currentPhaseDial->setValue(std::get<int>(val));},
        config::Source::Current::DefaultPhaseOffset
    };
    m_settingsMap["timeScale"] = {
        [this] { return m_ui->timeScaleWidget->value(); },
        [this](const SettingValue& val) { m_ui->timeScaleWidget->setValue(std::get<double>(val));},
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [this] { return m_ui->samplingCyclesControl->value(); },
        [this](const SettingValue& val) { m_ui->samplingCyclesControl->setValue(std::get<double>(val));},
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [this] { return static_cast<int>(m_ui->samplesPerCycleControl->value());},
        [this](const SettingValue& val) { m_ui->samplesPerCycleControl->setValue(std::get<int>(val));},
        config::Sampling::DefaultSamplesPerCycle
    };
    m_settingsMap["maxDataSize"] = {
        [this] { return m_engine->getMaxDataSize(); },
        [this](const SettingValue& val) { m_engine->applySettings(std::get<int>(val));},
        config::Simulation::DefaultDataSize
    };
    m_settingsMap["graphWidthSec"] = {
        [this] { return m_ui->graphViewPlaceholder->getGraphWidth();},
        [this](const SettingValue& val) { m_ui->graphViewPlaceholder->setGraphWidth(std::get<double>(val));},
        config::View::GraphWidth::Default
    };

    m_settingsMap["updateMode"] = {
        [this]() -> SettingValue {
            if(m_ui->perHalfCycleRadioButton->isChecked()) return 1;
            if(m_ui->PerCycleRadioButton->isChecked()) return 2;
            return 0;
        },
        [this](const SettingValue& val) {
            int mode = std::get<int>(val);
            if(mode == 1) m_ui->perHalfCycleRadioButton->setChecked(true);
            else if(mode == 2) m_ui->PerCycleRadioButton->setChecked(true);
            else m_ui->perSampleRadioButton->setChecked(true);
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

    m_ui->statusbar->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
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

    m_ui->statusbar->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
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
