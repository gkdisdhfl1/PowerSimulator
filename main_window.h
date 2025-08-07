#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <expected>

// 전방 선언
class SettingsDialog;
class SimulationEngine;
class ValueControlWidget;

class SettingsManager;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SimulationEngine *engine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleSettingButtonClicked();
    void onEngineRuninngStateChanged(bool isRunning);

    void onActionSaveSettings();
    void onActionLoadSettings();
    void onActionDeleteSettings();

private:
    using SettingValue = std::variant<int, double>;
    // getter는 int or double을 담은 variant를 반환
    using SettingGetter = std::function<SettingValue()>;
    // setter는 int 또는 double을 담은 variant를 받음
    using SettingSetter = std::function<void(const SettingValue&)>;

    struct SettingInfo {
        SettingGetter getter;
        SettingSetter setter;
        std::variant<int, double> defaultValue; // DB에 값이 없을 때 사용할 기본값
    };

    Ui::MainWindow *ui;
    SettingsDialog *m_settingsDialog;
    SimulationEngine *m_engine;

    std::unique_ptr<SettingsManager> m_settingsManager;
    std::map<std::string, SettingInfo> m_settingsMap;

    void setupUiWidgets();
    void createSignalSlotConnections();
    void initializeSettingsMap(); // 설정 맵을 초기화하는 함수
    std::expected<void, std::string> applySettingsToUi(std::string_view presetName); // 특정 프리셋을 UI에 적용하는 함수
    std::expected<void, std::string> saveUiToSettings(std::string_view presetName); // 현재 UI 상태를 특정 프리셋으로 저장하는 함수
};
#endif // MAIN_WINDOW_H
