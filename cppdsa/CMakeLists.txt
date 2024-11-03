cmake_minimum_required(VERSION 3.10)
project(CppDsa)

set(CMAKE_CXX_STANDARD 20)

# Add /usr/local to CMAKE_PREFIX_PATH to help CMake find spdlog
set(CMAKE_PREFIX_PATH "/usr/local")

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include spdlog
find_package(spdlog REQUIRED)  # Find spdlog package

# Specify the directory where your tests are located
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Use file(GLOB ...) to find all .cpp files in the tests directory
file(GLOB TEST_SOURCES "${TEST_DIR}/*.cpp")

# Add the source files
add_executable(CppDsaTest ${TEST_SOURCES} wub_unique_ptr.cpp)

# Link libraries
target_link_libraries(CppDsaTest ${GTEST_LIBRARIES} pthread spdlog::spdlog)  # Link spdlog
