# 1. "test_"로 시작하는 모든 cpp 파일 찾기
file(GLOB TEST_SOURCES "test_*.cpp")

foreach(TEST_FILE ${TEST_SOURCES})
    # 1. 파일 경로에서 이름만 추출
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    # 2. 각 테스트 파일마다 실행 파일 생성
    add_executable(${TEST_NAME} ${TEST_FILE})

    # 3. 핵심 로직과 Qt Test 라이브러리 연결
    target_link_libraries(${TEST_NAME} PRIVATE 
        PowerSimulatorCore
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
    )

    # 4. CTest에 등록
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    message(STATUS "Test added": ${TEST_NAME})
endforeach(TEST_FILE ${TEST_SOURCES})
