cmake_minimum_required(VERSION 3.10)
project(OrderMatchingEngine)

add_executable(OrderMatchingEngine src/main.cpp)

# If tests are enabled, add test executable
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(OrderMatchingEngineTests tests/test_main.cpp)
    target_link_libraries(OrderMatchingEngineTests gtest_main)
    add_test(NAME OrderMatchingEngineTests COMMAND OrderMatchingEngineTests)

    add_executable(EngineTests tests/test_engine.cpp)
    target_link_libraries(EngineTests gtest_main)
    add_test(NAME EngineTests COMMAND EngineTests)
endif()

add_custom_target(run
    COMMAND OrderMatchingEngine
    DEPENDS OrderMatchingEngine
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

add_custom_target(test_and_run
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND OrderMatchingEngine
    DEPENDS OrderMatchingEngine OrderMatchingEngine
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)