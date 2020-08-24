#!/bin/sh

########
# Default Clang build configuration of kuksa-val
# Additional CMake build parameters can be passed to the script when invoking

# Make or goto out-of-source build Directory
mkdir -p build
cd build

# Configure build
CC=clang CXX=clang++ cmake "$@" ..

# Build
make -j8
