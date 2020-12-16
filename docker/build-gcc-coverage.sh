#!/bin/sh

########
# Build and generate coverage based on unit-tests by default

# Make or goto out-of-source build Directory
mkdir -p build_test
cd build_test

# Configure build
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

# Build
make -j8

# goto and run unit-tests
ctest

mkdir -p coverage-report

# generate coverage information with gcovr
gcovr -v --html -r ../src/ src/  -o coverage-report/coverage.html
