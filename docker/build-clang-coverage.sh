#!/bin/sh

########
# Build and generate coverage based on unit-tests by default

cd kuksa.invehicle/w3c-visserver-api

# Make or goto out-of-source build Directory
mkdir -p build
cd build

# Configure build
CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

# make everything needed
make -j

mkdir -p coverage-report

# goto and run unit-tests
cd unit-test
./w3c-unit-test || true

# convert raw coverage data to indexed
llvm-profdata merge -sparse default.profraw -o default.profdata

# generate coverage information with llvm-cov
llvm-cov show  --format=html ../src/libw3c-visserver-core.so -instr-profile=default.profdata > ../coverage-report/coverage.html
