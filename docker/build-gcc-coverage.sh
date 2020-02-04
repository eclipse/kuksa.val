#!/bin/sh

########
# Build and generate coverage based on unit-tests by default

cd kuksa.invehicle/w3c-visserver-api

# Make or goto out-of-source build Directory
mkdir -p build
cd build

# Configure build
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

# Build
make -j

# goto and run unit-tests
cd unit-test
./w3c-unit-test

cd ..
mkdir -p coverage-report

# generate coverage information with gcovr
gcovr -v --html -r ../src/ src/  -o coverage-report/coverage.html
