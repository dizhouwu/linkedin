cmake_minimum_required(VERSION 3.10)
project(UniquePtrTest)

set(CMAKE_CXX_STANDARD 20)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add the source files
add_executable(UniquePtrTest tests/test_wub_unique_ptr.cpp wub_unique_ptr.cpp)
target_link_libraries(UniquePtrTest ${GTEST_LIBRARIES} pthread)
