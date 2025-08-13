#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <sqlite_modern_cpp.h>
#include <expected>
#include <sstream>

class SettingsManager
{
public:
    explicit SettingsManager(std::string_view dp_path);

    // 반환 타입을 std::expected로 변경
    using Error = std::string;

    // 설정을 저장하는 template 함수
    template<typename T>
    std::expected<void, Error> saveSetting(std::string_view preset_name, std::string_view key, const T& value) {
        try {
            std::stringstream ss;
            ss << value;
            db << "INSERT OR REPLACE INTO Settings (preset_name, key, value) VALUES (?, ?, ?);"
               << std::string(preset_name) << std::string(key) << ss.str();
            return {}; // 성공 시 void를 의미하는 빈 객체 반환
        } catch(const std::exception& e) {
            return std::unexpected("설정 저장 실패: " + std::string(e.what()));
        }
    }

    // 설정을 불러오는 template 함수
    // 키가 존재하지 않으면 defaultValue를 반환
    template<typename T>
    std::expected<T, Error> loadSetting(std::string_view preset_name, std::string_view key, const T& defaultValue) {
        try {
            std::string value_str;
            db << "SELECT value FROM Settings WHERE preset_name = ? AND key = ?;"
               << std::string(preset_name) << std::string(key) >> value_str;


            if(value_str.empty()) {
                return defaultValue; // 키가 없는 것은 오류가 아니므로 기본값 반환
            }

            T result;
            std::stringstream ss(value_str);
            ss >> result;
            if(ss.fail() || !ss.eof()) {
                return std::unexpected("'" + std::string(key) + "' 값을 반환하는데 실패했습니다.");
            }
            return result;
        } catch (const sqlite::sqlite_exception&) {
            // 행이 없는 경우(not found)는 비정상적인 상황으로 보고 기본값 반환
            return defaultValue;
        } catch(const std::exception& e) {
            return std::unexpected("설정 불러오기 실패: " + std::string(e.what()));
        }
    }

    std::expected<std::vector<std::string>, Error> getAllPresetNames();
    std::expected<void, Error> deletePreset(std::string_view preset_name);
    std::expected<void, Error> renamePreset(std::string_view old_name, std::string_view new_name);
private:
    sqlite::database db; // 데이터베이스 객체
};

#endif // SETTINGS_MANAGER_H
