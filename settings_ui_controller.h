#ifndef SETTINGS_UI_CONTROLLER_H
#define SETTINGS_UI_CONTROLLER_H

#include "ui_main_window.h"
#include "settings_manager.h"
#include "simulation_engine.h"


class SettingsUiController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsUiController(Ui::MainWindow* ui, SettingsManager& settingsManager, SimulationEngine* engine, QWidget* parent);

    // MainWindow의 액션에 연결될 public 함수들
    void handleSaveAction();
    void handleLoadAction();
    void handleDeleteAction();

private:
    using SettingValue = std::variant<int, double>;
    using SettingGetter = std::function<SettingValue()>;
    using SettingSetter = std::function<void(const SettingValue&)>;

    struct SettingInfo {
        SettingGetter getter;
        SettingSetter setter;
        SettingValue defaultValue; // DB에 값이 없을 때 사용할 기본값
    };

    Ui::MainWindow* m_ui;
    SettingsManager& m_settingsManager;
    SimulationEngine* m_engine;
    QWidget* m_parent;
    std::map<std::string, SettingInfo> m_settingsMap;

    void initializeSettingsMap(); // 설정 맵을 초기화하는 함수
    std::expected<void, std::string> applySettingsToUi(std::string_view presetName); // 특정 프리셋을 UI에 적용하는 함수
    std::expected<void, std::string> saveUiToSettings(std::string_view presetName); // 현재 UI 상태를 특정 프리셋으로 저장하는 함수
    std::optional<QString> promptUserWithPresetList(const QString& title, const QString& label);
};

#endif // SETTINGS_UI_CONTROLLER_H
