#!/bin/bash

# Build the project using CMake
mkdir -p build
cd build
cmake ..
make

# Run the C++ executable
./arrow_shm_boost

# Run the Python script
python ../read_arrow_shm.py

cd ..