cmake_minimum_required(VERSION 3.16)

# 1. 명시적으로 테스트 소스 파일 나열
set(TEST_SOURCES
    test_min_max.cpp
    test_analysis_utils.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
    # 1. 파일 경로에서 이름만 추출
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # 2. 각 테스트 파일마다 실행 파일 생성
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # 3. 핵심 로직과 Qt Test 라이브러리 연결
    target_link_libraries(${TEST_NAME} PRIVATE 
        PowerSimulatorCore
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    )

    # 4. CTest에 등록
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    message(STATUS "Test added": ${TEST_NAME})
endforeach()
