cmake_minimum_required(VERSION 3.16)

project(PowerSimulator VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)

# =================================================
# 외부 라이브러리 (SQLite, sqplite_modern_cpp)
# =================================================
include(FetchContent)

FetchContent_Declare(
    sqlite_modern_cpp
    GIT_REPOSITORY https://github.com/SqliteModernCpp/sqlite_modern_cpp.git
    GIT_SHALLOW TRUE
    GIT_BRANCH main
)
FetchContent_Declare(
    sqlite3
    URL         https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
    URL_HASH    SHA256=1D3049DD0F830A025A53105FC79FD2AB9431AEA99E137809D064D8EE8356B032
)

FetchContent_MakeAvailable(sqlite_modern_cpp)
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite_modern_cpp INTERFACE)
target_include_directories(sqlite_modern_cpp INTERFACE
    ${sqlite_modern_cpp_SOURCE_DIR}/hdr)

add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
# =================================================

set(PROJECT_SOURCES
        main.cpp
        main_window.cpp
        main_window.h
        main_window.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PowerSimulator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        graph_window.h graph_window.cpp graph_window.ui
        settings_dialog.h settings_dialog.cpp settings_dialog.ui
        simulation_engine.h simulation_engine.cpp
        data_point.h
        config.h
        fine_tuning_dial.h
        fine_tuning_dial.cpp
        value_control_widget.h value_control_widget.cpp value_control_widget.ui
        custom_chart_view.h
        custom_chart_view.cpp
        resources.qrc
        settings_manager.h settings_manager.cpp

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET PowerSimulator APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(PowerSimulator SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(PowerSimulator
            ${PROJECT_SOURCES}
        )
    endif()
endif()



target_include_directories(PowerSimulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(PowerSimulator PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    sqlite_modern_cpp
    sqlite3_lib
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.PowerSimulator)
endif()
set_target_properties(PowerSimulator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS PowerSimulator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PowerSimulator)
endif()
