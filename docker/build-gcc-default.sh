#!/bin/sh

########
# Default GCC build configuration of w3c-visserver
# Additional CMake build parameters can be passed to the script when invoking

cd kuksa.invehicle/w3c-visserver-api

# Make or goto out-of-source build Directory
mkdir -p build
cd build

# Configure build
CC=gcc CXX=g++ cmake "$@" ..

# Build
make -j
