#include "settings_manager.h"

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
        throw std::runtime_error("데이터베이스 초기화 실패: " + std::string(e.what()));
    }
}

std::expected<std::vector<std::string>, SettingsManager::Error> SettingsManager::getAllPresetNames() {
    try {
        std::vector<std::string> names;
        // DISTINCT를 사용해 중복되지 않는 preset_name 목록을 가져옴
        db << "SELECT DISTINCT preset_name FROM Settings;" >>
            [&](std::string name) {
                names.push_back(name);
            };
        return names;
    } catch(const std::exception& e) {
        return std::unexpected("프리셋 목록 불러오기 실패: " + std::string(e.what()));
    }
}

std::expected<void, SettingsManager::Error> SettingsManager::deletePreset(std::string_view preset_name) {
    try {
        db << "DELETE FROM Settings WHERE preset_name = ?;" << std::string(preset_name);
        return {};
    } catch(const std::exception& e) {
        return std::unexpected("프리셋 삭제 실패: " + std::string(e.what()));
    }
}
