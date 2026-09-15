# Unit test execution
add_executable(zeus_tests
    core/test_config.cpp
    core/test_logger.cpp
)


# Link GTest and the core library
target_link_libraries(zeus_tests
    PRIVATE zeus_core
    GTest::gtest
    GTest::gtest_main
)

# Discover and run tests automatically
gtest_discover_tests(zeus_tests)