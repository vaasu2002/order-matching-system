cmake_minimum_required(VERSION 3.10)

project(OrderMatchingEngine)

# Gather all source and header files in src/
file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/*.h src/*.hpp)

add_executable(OrderMatchingEngine ${SOURCES} ${HEADERS})

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

    file(GLOB TEST_SOURCES tests/*.cpp)
    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src} ${HEADERS})
        target_link_libraries(${test_name} gtest_main)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

add_custom_target(run
    COMMAND OrderMatchingEngine
    DEPENDS OrderMatchingEngine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(test_and_run
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND OrderMatchingEngine
    DEPENDS OrderMatchingEngine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)