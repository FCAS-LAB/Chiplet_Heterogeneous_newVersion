find_package(GTest REQUIRED)

# Enable testing
enable_testing()

option(ENABLE_LONG_TIME_CASE "Enable test cases that cost oong time to run." ON)

#add_subdirectory(env)       # Builds the library
#add_subdirectory(sideband)  # Builds the tests
file(GLOB_RECURSE TEST_SOURCES "./*.cpp")

add_executable(run_tests ${TEST_SOURCES})

if(ENABLE_LONG_TIME_CASE)
    target_compile_definitions(run_tests PRIVATE ENABLE_LONG_TIME_CASE_=1)
else()
    target_compile_definitions(run_tests PRIVATE ENABLE_LONG_TIME_CASE_=0)
endif()

# Link with libraries
target_link_libraries(run_tests PRIVATE
    PRIVATE
    GTest::gtest_main
    ccps
)

#add_test(NAME sideband_switcher_test COMMAND sideband_switcher_test)