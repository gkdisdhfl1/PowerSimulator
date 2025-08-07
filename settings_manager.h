#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <sqlite_modern_cpp.h>

class SettingsManager
{
public:
    explicit SettingsManager(std::string_view dp_path);

    // 설정을 저장하는 template 함수
    template<typename T> void saveSetting(std::string_view preset_name, std::string_view key, const T& value);

    // 설정을 불러오는 template 함수
    // 키가 존재하지 않으면 defaultValue를 반환
    template<typename T> T loadSetting(std::string_view preset_name, std::string_view key, const T& defaultValue);

    std::vector<std::string>getAllPresetNames();
    void deletePreset(std::string_view preset_name);

private:
    sqlite::database db; // 데이터베이스 객체
};

#endif // SETTINGS_MANAGER_H
