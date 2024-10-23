cmake_minimum_required(VERSION 3.10)
project(corral_example)

set(CMAKE_CXX_STANDARD 20)

# Add executable and link it with necessary libraries
add_executable(hello_world_corral hello_world_corral.cpp)

# Include third-party headers
include_directories(../third_party)

# Link boost and other necessary libraries
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(hello_world_corral Boost::system)
