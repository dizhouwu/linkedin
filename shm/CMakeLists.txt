cmake_minimum_required(VERSION 3.1)
project(ArrowExample)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_compile_options(-D_GLIBCXX_USE_CXX11_ABI=0)

# Find Apache Arrow package
find_package(Arrow REQUIRED)

# Specify the Boost root directory, relative to the base directory
set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_82_0")
set(Boost_INCLUDE_DIRS "${BOOST_ROOT}/include")
set(Boost_LIBRARY_DIRS "${BOOST_ROOT}/lib")

# Find Boost libraries
find_package(Boost 1.82 REQUIRED COMPONENTS system iostreams)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Add your executable
add_executable(arrow_shm_boost write_arrow_shm.cpp)

# Link Arrow, Boost, and rt libraries
target_link_libraries(arrow_shm_boost 
    PRIVATE 
    Arrow::arrow_shared
    ${Boost_LIBRARIES}
    rt
)