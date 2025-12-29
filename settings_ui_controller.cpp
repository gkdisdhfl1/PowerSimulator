#include "settings_ui_controller.h"
#include "control_panel.h"
#include "settings_dialog.h"
#include "settings_manager.h"
#include "three_phase_dialog.h"
#include "UIutils.h"
#include <QInputDialog>
#include <QStatusBar>

namespace {
constexpr int StatusBarTimeOut = 3000;
}

SettingsUiController::SettingsUiController(ControlPanel* controlPanel, SettingsManager& settingsManager, QWidget* parent)
    : QObject(parent)
    ,m_controlPanel(controlPanel)
    ,m_settingsManager(settingsManager)
    ,m_parent(parent)
{
    m_settingsDialog = std::make_unique<SettingsDialog>(this, m_parent);
    m_harmonicsDialog = std::make_unique<HarmonicsDialog>(m_parent);

    initializeSettingsMap();
    initializeControlPanelDefaultValues();
    initializeConnections();
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

    // maxDataSize 별도 처리
    auto maxDataSizeRes = m_settingsManager.loadSetting(presetName.toStdString(), "maxDataSize", config::Simulation::DataSize::DefaultDataSize);
    if(maxDataSizeRes) {
        previewData["저장 크기"] = *maxDataSizeRes;
    }

    emit presetValuesFetched(previewData);
}

void SettingsUiController::onApplyDialogSettings(const int maxDatasize, const int graphWidth)
{
    // 데이터 유실 확인
    // if(!requestMaxSizeChange(maxDatasize)) return;

    m_state.simulation.maxDataSize = maxDatasize;
    m_state.view.graphWidth = graphWidth;

    qDebug() << "state maxDataSize: " << m_state.simulation.maxDataSize;
    qDebug() << "state graphWidth: " << m_state.view.graphWidth;

    emit setMaxDataSize(maxDatasize);
    emit setGraphWidth(graphWidth);
}

void SettingsUiController::onAmplitudeChanged(double value)
{
    m_state.source.amplitude = value;
    emit setAmplitude(value);
}
void SettingsUiController::onCurrentAmplitudeChanged(double value)
{
    m_state.source.currentAmplitude = value;
    emit setCurrentAmplitude(value);
}
void SettingsUiController::onFrequencyChanged(double value)
{
    m_state.source.frequency = value;
    emit setFrequency(value);
    if(!m_blockUiSignals)
        requestCaptureIntervalUpdate();
}
void SettingsUiController::onCurrentPhaseChanged(int degrees)
{
    m_state.source.currentPhaseDegrees = degrees;
    emit setCurrentPhase(utils::degreesToRadians(degrees));
}
void SettingsUiController::onTimeScaleChanged(double value)
{
    m_state.simulation.timeScale = value;
    emit setTimeScale(value);
    if(!m_blockUiSignals)
        emit requestCaptureIntervalUpdate();
}
void SettingsUiController::onSamplingCyclesChanged(double value)
{
    m_state.simulation.samplingCycles = value;
    emit setSamplingCycles(value);
    if(!m_blockUiSignals)
        emit requestCaptureIntervalUpdate();
}
void SettingsUiController::onSamplesPerCycleChanged(int value)
{
    m_state.simulation.samplesPerCycle = value;
    emit setSamplesPerCycle(value);
    if(!m_blockUiSignals)
        emit requestCaptureIntervalUpdate();
}
void SettingsUiController::onUpdateModeChanged(UpdateMode mode)
{
    m_state.simulation.updateMode = mode;
    emit setUpdateMode(mode);
}

void SettingsUiController::onTrackingToggled(bool enabled)
{
    emit enableTracking(enabled);
}

void SettingsUiController::showSettingsDialog()
{
    m_settingsDialog->openWithValues(m_state);
}

void SettingsUiController::onCoefficientsChanged(const FrequencyTracker::PidCoefficients& fllCoeffs, const FrequencyTracker::PidCoefficients& zcCoeffs)
{
    emit setFrequencyTrackerCoefficients(fllCoeffs, zcCoeffs);
}

void SettingsUiController::onHarmonicsSettingsRequested()
{
    if(!m_harmonicsDialog) return;

    // 다이얼로그를 열기 전에 엔진의 현재 고조파 리스트로 UI 동기화
    m_harmonicsDialog->setHarmonics(m_state.harmonics.voltageList, m_state.harmonics.currentList);

    m_harmonicsDialog->show();
    m_harmonicsDialog->raise();
    m_harmonicsDialog->activateWindow();
}

void SettingsUiController::onThreePhaseValueChanged(int type, double value)
{
    // 매핑 구조체 정의
    struct Mapping {
        double ControlPanelState::ThreePhase::*memberPtr; // State 멤버 포인터
        std::function<void(double)> signalEmitter; // 시그널 방출 람다
    };

    static const std::unordered_map<int, Mapping> propertyMap = {
        {ThreePhaseDialog::VoltageBAmplitude, {&ControlPanelState::ThreePhase::voltageBAmplitude, [this](double v) {emit setVoltageBAmplitude(v);}}},
        {ThreePhaseDialog::VoltageBPhase, {&ControlPanelState::ThreePhase::voltageBPhase, [this](double v) {emit setVoltageBPhase(v);}}},
        {ThreePhaseDialog::VoltageCAmplitude, {&ControlPanelState::ThreePhase::voltageCAmplitude, [this](double v) {emit setVoltageCAmplitude(v);}}},
        {ThreePhaseDialog::VoltageCPhase, {&ControlPanelState::ThreePhase::voltageCPhase, [this](double v) {emit setVoltageCPhase(v);}}},
        {ThreePhaseDialog::CurrentBAmplitude, {&ControlPanelState::ThreePhase::currentBAmplitude, [this](double v) {emit setCurrentBAmplitude(v);}}},
        {ThreePhaseDialog::CurrentBPhase, {&ControlPanelState::ThreePhase::currentBPhase, [this](double v) {emit setCurrentBPhase(v);}}},
        {ThreePhaseDialog::CurrentCAmplitude, {&ControlPanelState::ThreePhase::currentCAmplitude, [this](double v) {emit setCurrentCAmplitude(v);}}},
        {ThreePhaseDialog::CurrentCPhase, {&ControlPanelState::ThreePhase::currentCPhase, [this](double v) {emit setCurrentCPhase(v);}}},
    };

    auto it = propertyMap.find(type);
    if(it != propertyMap.end()) {
        (m_state.threePhase.*(it->second.memberPtr)) = value;
        it->second.signalEmitter(value);
    }
}

// -------------------------

// ----- private 헬퍼 함수들 ------
void SettingsUiController::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록

    // 일반 값 바인딩
    // bind("키", m_state 변수, &Controller::시그널, 기본값, "이름");

    bind("voltageAmplitude", m_state.source.amplitude, &SettingsUiController::setAmplitude, config::Source::Amplitude::Default, "진폭 (Voltage)");
    bind("currentAmplitude", m_state.source.currentAmplitude, &SettingsUiController::setCurrentAmplitude, config::Source::Current::DefaultAmplitude, "진폭 (Current)");
    bind("frequency", m_state.source.frequency, &SettingsUiController::setFrequency, config::Source::Frequency::Default, "주파수");
    bindAngle("currentPhaseOffset", m_state.source.currentPhaseDegrees, &SettingsUiController::setCurrentPhase, config::Source::Current::DefaultPhaseOffset, "위상차");
    bind("timeScale", m_state.simulation.timeScale, &SettingsUiController::setTimeScale, config::TimeScale::Default, "시간 배율");
    bind("samplingCycles", m_state.simulation.samplingCycles, &SettingsUiController::setSamplingCycles, config::Sampling::DefaultSamplingCycles, "초당 cycle");
    bind("samplesPerCycle", m_state.simulation.samplesPerCycle, &SettingsUiController::setSamplesPerCycle, config::Sampling::DefaultSamplesPerCycle, "cycle당 sample");
    bind("graphWidthSec", m_state.view.graphWidth, &SettingsUiController::setGraphWidth, config::Simulation::GraphWidth::Default, "그래프 시간 폭");
    bind("updateMode", m_state.simulation.updateMode, &SettingsUiController::setUpdateMode, 2, "갱신 모드");

    // 3상 관련 설정
    bind("voltageBAmplitude", m_state.threePhase.voltageBAmplitude, &SettingsUiController::setVoltageBAmplitude, config::Source::ThreePhase::DefaultAmplitudeB, "B상 전압 크기");
    bindAngle("voltageBPhase", m_state.threePhase.voltageBPhase, &SettingsUiController::setVoltageBPhase, config::Source::ThreePhase::DefaultPhaseB_deg, "B상 전압 위상");
    bind("voltageCAmplitude", m_state.threePhase.voltageCAmplitude, &SettingsUiController::setVoltageCAmplitude, config::Source::ThreePhase::DefaultAmplitudeC, "C상 전압 크기");
    bindAngle("voltageCPhase", m_state.threePhase.voltageCPhase, &SettingsUiController::setVoltageCPhase, config::Source::ThreePhase::DefaultPhaseC_deg, "C상 전압 위상");

    bind("currentBAmplitude", m_state.threePhase.currentBAmplitude, &SettingsUiController::setCurrentBAmplitude, config::Source::ThreePhase::DefaultCurrentAmplitudeB, "B상 전류 크기");
    bindAngle("currentBPhase", m_state.threePhase.currentBPhase, &SettingsUiController::setCurrentBPhase, config::Source::ThreePhase::DefaultCurrentPhaseB_deg, "B상 전류 위상");
    bind("currentCAmplitude", m_state.threePhase.currentCAmplitude, &SettingsUiController::setCurrentCAmplitude, config::Source::ThreePhase::DefaultCurrentAmplitudeC, "C상 전류 크기");
    bindAngle("currentCPhase", m_state.threePhase.currentCPhase, &SettingsUiController::setCurrentCPhase, config::Source::ThreePhase::DefaultCurrentPhaseC_deg, "C상 전류 위상");
}

void SettingsUiController::initializeControlPanelDefaultValues()
{
    // DTO를 통한 일괄 주입
    m_controlPanel->setState(m_state);

    emit setVoltageHarmonics(m_state.harmonics.voltageList);
    emit setVoltageHarmonics(m_state.harmonics.currentList);
}

void SettingsUiController::initializeConnections()
{
    connect(m_harmonicsDialog.get(), &HarmonicsDialog::voltageHarmonicsChanged, this, [this](const HarmonicList& list) {
        m_state.harmonics.voltageList = list; // 내부 state 업데이트
        emit setVoltageHarmonics(list); // 엔진에게 변경 알림
    });
    connect(m_harmonicsDialog.get(), &HarmonicsDialog::currentHarmonicsChanged, this, [this](const HarmonicList& list) {
        // 내부 state 업데이트
        m_state.harmonics.currentList = list;
        // 엔진에게 변경 알림
        emit setCurrentHarmonics(list);
    });
}

bool SettingsUiController::requestMaxSizeChange(int newSize)
{
    const int currentDataSize = m_state.simulation.maxDataSize;

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
    if(auto res = m_settingsManager.loadSetting(presetName, "maxDataSize", m_state.simulation.maxDataSize); res) {
        if(requestMaxSizeChange(*res)) {
            m_state.simulation.maxDataSize = *res;
            emit setMaxDataSize(*res);
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

        // setter
        info.setter(loadedValue);
    }
    // 고조파 리스트 로드
    auto voltageRes = m_settingsManager.loadSetting(presetName, "voltageHarmonicsJson", std::string(""));
    if(voltageRes) {
        const auto& voltageList = UIutils::jsonToHarmonicList(QString::fromStdString(*voltageRes));
        m_state.harmonics.voltageList = voltageList;
        emit setVoltageHarmonics(voltageList);
    }
    auto currentRes = m_settingsManager.loadSetting(presetName, "currentHarmonicsJson", std::string(""));
    if(currentRes) {
        const auto& currentList = UIutils::jsonToHarmonicList(QString::fromStdString(*currentRes));
        m_state.harmonics.currentList = currentList;
        emit setCurrentHarmonics(currentList);
    }

    m_blockUiSignals = false;
    emit presetApplied();
    emit requestCaptureIntervalUpdate(); // 엔진 파라미터 재계산 요청
    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data() , presetName.size())), StatusBarTimeOut);
    return {};
}

std::expected<void, std::string> SettingsUiController::saveEngineToSettings(std::string_view presetName)
{
    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        // 1. Getter로 현재 값을 QVariant로 가져옴
        QVariant valueToSave = info.getter();

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
    // 고조파 리스트 저장
    QString voltageJson = UIutils::harmonicListToJson(m_state.harmonics.voltageList);
    QString currentJson = UIutils::harmonicListToJson(m_state.harmonics.currentList);

    if(auto res = m_settingsManager.saveSetting(presetName, "voltageHarmonicsJson", voltageJson.toStdString()); !res) return res;
    if(auto res = m_settingsManager.saveSetting(presetName, "currentHarmonicsJson", currentJson.toStdString()); !res) return res;

    // UI와 별개인 엔진 파라미터들도 저장
    if(auto res = m_settingsManager.saveSetting(presetName, "maxDataSize", m_state.simulation.maxDataSize); !res) return res;

    m_parent->findChild<QStatusBar*>()->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), StatusBarTimeOut);
    return {};
}
