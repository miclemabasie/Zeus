cmake_minimum_required(VERSION 3.20)

project(Zeus VERSION 0.1.0 LANGUAGE CXX)

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Hardcore warning flags for production quality
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpendantic -Werror Wno-unused-parameter")
endif()

# option for building tests
option(BUILD_TESTS, "Build unit tests" ON)

# Find dependencies
find_package(nlohman_json CONFIG REQUIRED)
if(BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
endif()


# Include directories globally
include_directories(include)

# Add the source subdirectory
add_subdirectory(src)

# Add tests if enabled
if(BUILD_TESTS) 
    add_subdirectory(tests)
endif()

# Add a convenience target to run all tests
if(BUILD_TESTS)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)
endif()
