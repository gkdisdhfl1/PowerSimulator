#include "settings_ui_controller.h"
#include "control_panel.h"
#include "UIconfig.h"
#include "settings_dialog.h"
#include "settings_manager.h"
#include "simulation_engine.h"
#include "three_phase_dialog.h"
#include <QInputDialog>
#include <QStatusBar>

namespace {
constexpr int StatusBarTimeOut = 3000;
}

SettingsUiController::SettingsUiController(ControlPanel* controlPanel, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent)
    : QObject(parent)
    ,m_controlPanel(controlPanel)
    ,m_settingsManager(settingsManager)
    ,m_engine(engine)
    ,m_parent(parent)
    // ,m_voltageHarmonicOrderAdapter(engine->m_voltageHarmonic, &HarmonicComponent::order)
    // ,m_voltageHarmonicMagnitudeAdapter(engine->m_voltageHarmonic, &HarmonicComponent::magnitude)
    // ,m_voltageHarmonicPhaseAdapter(engine->m_voltageHarmonic, &HarmonicComponent::phase)
    // ,m_currentHarmonicOrderAdapter(engine->m_currentHarmonic, &HarmonicComponent::order)
    // ,m_currentHarmonicMagnitudeAdapter(engine->m_currentHarmonic, &HarmonicComponent::magnitude)
    // ,m_currentHarmonicPhaseAdapter(engine->m_currentHarmonic, &HarmonicComponent::phase)
{
    initializeSettingsMap();

    engine->m_graphWidthSec.setValue(View::GraphWidth::Default);
    m_settingsDialog = std::make_unique<SettingsDialog>(this, m_parent);

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

        QVariant loadedValueAsVariant;
        const QVariant& defaultValue = info.defaultValue;

        // defaultValue의 타입에 따라 loadSetting을 다르게 호출
        if(defaultValue.typeId() == QMetaType::Int) {
            auto result = m_settingsManager.loadSetting(presetStdString, key, defaultValue.toInt());
            if(result) loadedValueAsVariant = *result;
        } else if(defaultValue.typeId() == QMetaType::Double) {
            auto result = m_settingsManager.loadSetting(presetStdString, key, defaultValue.toDouble());
            if(result) loadedValueAsVariant = *result;
        } else {
            // 기본적으로 문자열로 처리 시도
            auto result = m_settingsManager.loadSetting(presetStdString, key, defaultValue.toString().toStdString());
            if(result) loadedValueAsVariant = QString::fromStdString(*result);
        }

        // QVariant가 유효할 때에만 맵에 추가
        if(loadedValueAsVariant.isValid()) {
            if(info.displayName.isEmpty()) continue;

            // updateMode의 경우, 숫자 대신 문자열로 반환하여 표시
            if(QString::fromStdString(key) == "updateMode") {
                int mode = loadedValueAsVariant.toInt();
                QString modeStr = (mode == 0) ? "Per Sample" : (mode == 1) ? "Per Half Cycle" : "Per Cycle";
                previewData[info.displayName] = modeStr;
            } else {
                previewData[info.displayName] = loadedValueAsVariant;
            }
        } else {
            qWarning() << "Could not load setting" << QString::fromStdString(key)
                           << "for preset" << presetName;
        }
    }

    emit presetValuesFetched(previewData);
}

void SettingsUiController::onApplyDialogSettings(const int maxDatasize, const int graphWidth)
{
    if(!m_engine) return;

    if(!requestMaxSizeChange(maxDatasize)) return;
    emit maxDataSizeChangeRequested(maxDatasize);
    m_engine->m_graphWidthSec.setValue(graphWidth);
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
    m_settingsDialog->openWithValues(m_engine);
}

void SettingsUiController::onTrackingToggled(bool enabled)
{
    m_engine->enableFrequencyTracking(enabled);
}


void SettingsUiController::onCoefficientsChanged(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs)
{
    m_engine->getFrequencyTracker()->setFllCoefficients(fllCoeffs);
    m_engine->getFrequencyTracker()->setZcCoefficients(zcCoeffs);
}

// void SettingsUiController::onHarmonicsChanged()
// {
//     const auto state = m_controlPanel->getState();
//     auto& params = m_engine;

//     params->m_voltageHarmonic.setValue(state.voltageHarmonic);
//     params->m_currentHarmonic.setValue(state.currentHarmonic);
// }

void SettingsUiController::onThreePhaseValueChanged(int type, double value)
{
    static const std::unordered_map<int, Property<double> SimulationEngine::*> propertyMap = {
        {ThreePhaseDialog::VoltageBAmplitude, &SimulationEngine::m_voltage_B_amplitude},
        {ThreePhaseDialog::VoltageBPhase, &SimulationEngine::m_voltage_B_phase_deg},
        {ThreePhaseDialog::VoltageCAmplitude, &SimulationEngine::m_voltage_C_amplitude},
        {ThreePhaseDialog::VoltageCPhase, &SimulationEngine::m_voltage_C_phase_deg},
        {ThreePhaseDialog::CurrentBAmplitude, &SimulationEngine::m_current_B_amplitude},
        {ThreePhaseDialog::CurrentBPhase, &SimulationEngine::m_current_B_phase_deg},
        {ThreePhaseDialog::CurrentCAmplitude, &SimulationEngine::m_current_C_amplitude},
        {ThreePhaseDialog::CurrentCPhase, &SimulationEngine::m_current_C_phase_deg}
    };

    auto it = propertyMap.find(type);
    if(it != propertyMap.end()) {
        // 멤버 포인터를 통해 인스턴스의 멤버에 접근
        (m_engine->*(it->second)).setValue(value);
    }
}

// -------------------------

// ----- private 헬퍼 함수들 ------
void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    m_settingsMap["voltageAmplitude"] = {&m_engine->m_amplitude, config::Source::Amplitude::Default, "진폭 (Voltage)"};
    m_settingsMap["currentAmplitude"] = {&m_engine->m_currentAmplitude, config::Source::Current::DefaultAmplitude, "진폭 (Current)"};
    m_settingsMap["frequency"] = {&m_engine->m_frequency, config::Source::Frequency::Default, "주파수"};
    m_settingsMap["currentPhaseOffset"] = {&m_engine->m_currentPhaseOffsetRadians, config::Source::Current::DefaultPhaseOffset, "위상차", true};
    m_settingsMap["timeScale"] = {&m_engine->m_timeScale, config::TimeScale::Default, "시간 배율"};
    m_settingsMap["samplingCycles"] = {&m_engine->m_samplingCycles, config::Sampling::DefaultSamplingCycles, "초당 cycle"};
    m_settingsMap["samplesPerCycle"] = {&m_engine->m_samplesPerCycle, config::Sampling::DefaultSamplesPerCycle, "cycle당 sample"};
    m_settingsMap["graphWidthSec"] = {&m_engine->m_graphWidthSec, View::GraphWidth::Default, "그래프 시간 폭"};
    m_settingsMap["updateMode"] = {&m_engine->m_updateMode, 0, "갱신 모드"};

    // 고조파 관련 설정
    // m_settingsMap["voltHarmonicOrder"] = {&m_voltageHarmonicOrderAdapter, config::Harmonics::DefaultOrder, "전압 고조파 차수"};
    // m_settingsMap["voltHarmonicMagnitude"] = {&m_voltageHarmonicMagnitudeAdapter, config::Harmonics::DefaultMagnitude, "전압 고조파 크기"};
    // m_settingsMap["voltHarmonicPhase"] = {&m_voltageHarmonicPhaseAdapter, config::Harmonics::DefaultPhase, "전압 고조파 위상"};
    // m_settingsMap["currHarmonicOrder"] = {&m_currentHarmonicOrderAdapter, config::Harmonics::DefaultOrder, "전류 고조파 차수"};
    // m_settingsMap["currHarmonicMagnitude"] = {&m_currentHarmonicMagnitudeAdapter, config::Harmonics::DefaultMagnitude, "전류 고조파 크기"};
    // m_settingsMap["currHarmonicPhase"] = {&m_currentHarmonicPhaseAdapter, config::Harmonics::DefaultPhase, "전류 고조파 위상"};

    // 3상 관련 설정
    m_settingsMap["voltageBAmplitude"] = {&m_engine->m_voltage_B_amplitude, config::Source::ThreePhase::DefaultAmplitudeB, "B상 전압 크기"};
    m_settingsMap["voltageBPhase"] = {&m_engine->m_voltage_B_phase_deg, config::Source::ThreePhase::DefaultPhaseB_deg, "B상 전압 위상"};
    m_settingsMap["voltageCAmplitude"] = {&m_engine->m_voltage_C_amplitude, config::Source::ThreePhase::DefaultAmplitudeC, "C상 전압 크기"};
    m_settingsMap["voltageCPhase"] = {&m_engine->m_voltage_C_phase_deg, config::Source::ThreePhase::DefaultPhaseC_deg, "C상 전압 위상"};

    m_settingsMap["currentBAmplitude"] = {&m_engine->m_current_B_amplitude, config::Source::ThreePhase::DefaultCurrentAmplitudeB, "B상 전류 크기"};
    m_settingsMap["currentBPhase"] = {&m_engine->m_current_B_phase_deg, config::Source::ThreePhase::DefaultCurrentPhaseB_deg, "B상 전류 위상"};
    m_settingsMap["currentCAmplitude"] = {&m_engine->m_current_C_amplitude, config::Source::ThreePhase::DefaultCurrentAmplitudeC, "C상 전류 크기"};
    m_settingsMap["currentCPhase"] = {&m_engine->m_current_C_phase_deg, config::Source::ThreePhase::DefaultCurrentPhaseC_deg, "C상 전류 위상"};
}

bool SettingsUiController::requestMaxSizeChange(int newSize)
{
    const int currentDataSize = m_engine->getDataSize();

    // 새 최대 크기가 현재 데이터 개수보다 작을 경우
    if(newSize < currentDataSize) {
        bool ok = false;
        emit requestDataLossConfirmation(currentDataSize, newSize, &ok);
        if(!ok) return false; // 사용자가 취소함
    }

    return true;
}

std::expected<void, std::string> SettingsUiController::applySettingsToEngine(std::string_view presetName)
{
    m_blockUiSignals = true; // 프리셋 적용 시 슬롯 호출 잠시 무시

    // 미리 최대 데이터 크기 변경 요청
    if(auto res = m_settingsManager.loadSetting(presetName, "maxDataSize", m_engine->m_maxDataSize.value()); res) {
        if(requestMaxSizeChange(*res)) {
            emit maxDataSizeChangeRequested(*res);
        } else {
            m_blockUiSignals = false;
            return std::unexpected("사용자가 최대 데이터 크기 변경을 취소했습니다.");
        }
    } else {
        m_blockUiSignals = false;
        return std::unexpected(res.error());
    }

    // 맵을 순회하면 DB에서 값을 불러와 각 Property에 적용
    for(auto const& [key, info] : m_settingsMap) {
        // 1. DB에서 값을 문자열로 불러옴
        std::string defaultValueStr = info.defaultValue.toString().toStdString();

        auto loadResult = m_settingsManager.loadSetting(presetName, key, defaultValueStr);

        if(!loadResult) {
            // 실패 시, 에러를 그대로 반환
            m_blockUiSignals = false;
            return std::unexpected(loadResult.error());
        }

        // 2. 불러온 문자열을 QVariant로 변환
        QVariant loadedValue(QString::fromStdString(*loadResult));

        // 위상은 Degree -> Radian 변환
        if(info.isAngle) {
            double degrees = loadedValue.toDouble();
            double radians = utils::degreesToRadians(degrees);
            loadedValue = radians;
        }
            
        // 3. Property 인터페이서를 통해 값 설정
        info.property->setVariantValue(loadedValue);
    }

    m_blockUiSignals = false;
    emit presetApplied();
    m_engine->recalculateCaptureInterval(); // 엔진 파라미터 재계산
    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data() , presetName.size())), StatusBarTimeOut);
    return {};
}

std::expected<void, std::string> SettingsUiController::saveEngineToSettings(std::string_view presetName)
{
    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        // 1. Property 인터페이스를 통해 현재 값을 QVariant로 가져옴
        QVariant valueToSave = info.property->getVariantValue();

        // 위상 값은 Radian -> Degree 변환
        if(info.isAngle) {
            double radians = valueToSave.toDouble();
            double degrees = utils::radiansToDegrees(radians);
            valueToSave = degrees;
        }

        // 2. QVariant를 DB에 저장할 수 있는 형태로 변환
        std::string valueStr = valueToSave.toString().toStdString();

        // 3. DB에 저장
        auto result  = m_settingsManager.saveSetting(presetName, key, valueStr);

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    // UI와 별개인 엔진 파라미터들도 저장
    if(auto res = m_settingsManager.saveSetting(presetName, "maxDataSize", m_engine->m_maxDataSize.value()); !res) return res;

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), StatusBarTimeOut);
    return {};
}
