cmake_minimum_required(VERSION 3.16)

# =================================================
# 1. 프로젝트 설정 (Project Setup)
# =================================================
project(PowerSimulator VERSION 0.1 LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt Auto 설정
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# =================================================
# 2. 의존성 라이브러리 (Dependencies)
# =================================================

# 2.1 Qt 라이브러리 찾기
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts Core Test)

# 2.2 외부 라이브러리 (SQLite, sqlite_modern_cpp) - FetchContent
include(FetchContent)

# SQLite Modern C++ Wrapper
FetchContent_Declare(
    sqlite_modern_cpp
    GIT_REPOSITORY https://github.com/SqliteModernCpp/sqlite_modern_cpp.git
    GIT_SHALLOW TRUE
    GIT_BRANCH main
)

# SQLite3 Amalgamation
FetchContent_Declare(
    sqlite3
    URL         https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
    URL_HASH    SHA256=1D3049DD0F830A025A53105FC79FD2AB9431AEA99E137809D064D8EE8356B032
)

FetchContent_MakeAvailable(sqlite_modern_cpp sqlite3)

# SQLite target 설정
add_library(sqlite_modern_cpp INTERFACE)
target_include_directories(sqlite_modern_cpp INTERFACE ${sqlite_modern_cpp_SOURCE_DIR}/hdr)

add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})

# =================================================
# 3. 코어 라이브러리 (Core Logic Library)
# =================================================
# UI와 무관한 순수 로직 파일들
set(CORE_SOURCES
    # Engine & Manager
    simulation_engine.h simulation_engine.cpp
    settings_manager.h settings_manager.cpp
    a3700n_datasource_factory.h a3700n_datasource_factory.cpp

    # Data
    config.h
    data_point.h
    measured_data.h
    demand_data.h
    shared_data_types.h
    Property.h

    # Logic & Analysis
    frequency_tracker.h frequency_tracker.cpp
    analysis_utils.h analysis_utils.cpp
    min_max_tracker.h
    demand_calculator.h demand_calculator.cpp

    # Third-Party
    kiss_fft/kiss_fft.c kiss_fft/kiss_fftr.c
)

# 정적 라이브러리 생성
add_library(PowerSimulatorCore STATIC ${CORE_SOURCES})

# Core 라이브러리의 의존성 (UI 제외)
target_link_libraries(PowerSimulatorCore PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    sqlite_modern_cpp
    sqlite3_lib
)

# Core 헤더 경로 포함
target_include_directories(PowerSimulatorCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kiss_fft
)

# =================================================
# 4. 메인 실행 파일 (Application)
# =================================================
# UI 관련 소스 파일들
set (APP_UI_SOURCES
        # Core & Main
        main.cpp
        main_window.h main_window.cpp
        UIconfig.h
        datasource_types.h
        resources.qrc
        UIutils.h UIutils.cpp

        # UI Components & Dialogs
        graph_window.h graph_window.cpp
        settings_dialog.h settings_dialog.cpp
        settings_ui_controller.h settings_ui_controller.cpp
        control_panel.h control_panel.cpp
        control_panel_state.h
        value_control_widget.h value_control_widget.cpp
        fine_tuning_dial.h fine_tuning_dial.cpp
        collapsible_groupbox.h collapsible_groupbox.cpp
        three_phase_dialog.h three_phase_dialog.cpp
        pid_controller.h pid_controller.cpp
        pid_tuning_dialog.h pid_tuning_dialog.cpp
        data_row_widget.h data_row_widget.cpp

        # Charts & Visualization
        custom_chart_view.h custom_chart_view.cpp
        base_graph_window.h base_graph_window.cpp
        analysis_graph_window.h analysis_graph_window.cpp
        fundamental_analysis_graph_window.h fundamental_analysis_graph_window.cpp
        harmonic_analysis_graph_window.h harmonic_analysis_graph_window.cpp
        phasor_view.h phasor_view.cpp
        one_second_summary_window.h one_second_summary_window.cpp
        additional_metrics_window.h additional_metrics_window.cpp
        a3700n_window.h a3700n_window.cpp

        # Pages
        data_page.h data_page.cpp
        analysis_phasor_page.h analysis_phasor_page.cpp
        analysis_waveform_page.h analysis_waveform_page.cpp
        analysis_harmonic_page.h analysis_harmonic_page.cpp
)

# =================================================
# 5. 실행 파일 타겟 생성 (Executable Target)
# =================================================
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PowerSimulator
        MANUAL_FINALIZATION
        ${APP_UI_SOURCES}
)
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET PowerSimulator APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(PowerSimulator SHARED
            ${APP_UI_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(PowerSimulator
            ${APP_UI_SOURCES}
        )
    endif()
endif()

# 4.1 Core 라이브러리와 UI 라이브러리 연결
target_link_libraries(PowerSimulator PRIVATE
    PowerSimulatorCore
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
)

# 4.2 include directories
target_include_directories(PowerSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kiss_fft
)

# 4.3 속성 설정 (Bundle ID, Version 등)
set_target_properties(PowerSimulator PROPERTIES
    MACOSX_BUNDLE_IDENTIFIER com.example.PowerSimulator
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 4.4 설치 규칙
include(GNUInstallDirs)
install(TARGETS PowerSimulator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 4.5 Qt 6 Finalization
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PowerSimulator)
endif()

# =================================================
# 6. 테스트 서브 디렉토리 (Tests)
# =================================================

# CTest 활성화
enable_testing()
add_subdirectory(tests)
