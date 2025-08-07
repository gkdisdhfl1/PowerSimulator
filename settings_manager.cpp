#include "settings_manager.h"
#include <sstream>

SettingsManager::SettingsManager(std::string_view dp_path) : db(std::string(dp_path)) {
    try {
        // Settings 테이블이 없으면 새로  생성
        db << "CREATE TABLE IF NOT EXISTS Settings ("
              " preset_name TEXT NOT NULL,"
              " key TEXT NOT NULL,"
              " value TEXT NOT NULL,"
              " PRIMARY KEY (preset_name, key)"
              ");";
    } catch (const std::exception& e) {
        std::cerr << "Database Error: " << e.what() << std::endl;
    }
}

// saveSetting 템플릿 구현
template<typename T>
void SettingsManager::saveSetting(std::string_view preset_name, std::string_view key, const T& value) {
    std::stringstream ss;
    ss << value;
    std::string value_str = ss.str();

    // key를 std::string으로 변환하여 쿼리에 바인딩
    db << "INSERT OR REPLACE INTO Settings (preset_name, key, value) VALUES (?, ?, ?);"
       << std::string(preset_name) << std::string(key) << value_str;
}

// loadSetting 템플릿 구현
template<typename T>
T SettingsManager::loadSetting(std::string_view preset_name, std::string_view key, const T& defaultValue) {
    T result = defaultValue;
    try {
        std::string value_str;
        // key를 std::string으로 변환하여 쿼리에 바인딩
        db << "SELECT value FROM Settings WHERE preset_name = ? AND key = ?;"
           << std::string(preset_name) << std::string(key) >> value_str;

        if(!value_str.empty()) {
            std::stringstream ss;
            ss << value_str;
            ss >> result;
        }
    } catch (const sqlite::sqlite_exception&) {
        // 키가 존재하지 않는 경우 등 예외 발생 시 기본값 사용

    }
    return result;
}

std::vector<std::string> SettingsManager::getAllPresetNames() {
    std::vector<std::string> names;
    try {
        // DISTINCT를 사용해 중복되지 않는 preset_name 목록을 가져옴
        db << "SELECT DISTINCT preset_name FROM Settings;" >>
            [&](std::string name) {
                names.push_back(name);
            };
    } catch(const std::exception e) {
        std::cerr << "Error loading preset names: " << e.what() << std::endl;
    }
    return names;
}

void SettingsManager::deletePreset(std::string_view preset_name) {
    try {
        db << "DELETE FROM Settings WHERE preset_name = ?;" << std::string(preset_name);
    } catch(const std::exception e) {
        std::cerr << "Error deleting preset: " << e.what() << std::endl;
    }
}

template void SettingsManager::saveSetting<double>(std::string_view, std::string_view, const double&);
template double SettingsManager::loadSetting<double>(std::string_view, std::string_view, const double&);
template void SettingsManager::saveSetting(std::string_view, std::string_view, const int&);
template int SettingsManager::loadSetting(std::string_view, std::string_view, const int&);
