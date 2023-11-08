#!/bin/bash
#
# Copyright (c) 2023 Contributors to the Eclipse Foundation
#
# Building all currently supported targets for databroker-cli.
# Uses cross for cross-compiling. Needs to be executed
# before docker build, as docker collects the artifacts
# created by this script
# this needs the have cross, cargo-license and createbom dependencies installed
#
# SPDX-License-Identifier: Apache-2.0


# exit on error, to not waste any time
set -e

CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse

# Create thirdparty bom
cd createbom/
rm -rf ../databroker/thirdparty || true
python3 createbom.py ../databroker-cli
cd ..

# Starting a fresh build
echo "Cargo clean, to start fresh..."
cargo clean
rm -rf ../dist || true
mkdir ../dist

# Buidling AMD46
echo "Building AMD64"
cross build --target x86_64-unknown-linux-musl --bin databroker-cli --release
# We need to clean this folder in target, otherwise we get weird side
# effects building the aarch image, complaining libc crate can not find
# GLIBC, i.e
#   Compiling libc v0.2.149
#error: failed to run custom build command for `libc v0.2.149`
#
#Caused by:
#  process didn't exit successfully: `/target/release/build/libc-2dd22ab6b5fb9fd2/#build-script-build` (exit status: 1)
#  --- stderr
#  /target/release/build/libc-2dd22ab6b5fb9fd2/build-script-build: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.29' not found (required by /target/release/build/libc-2dd22ab6b5fb9fd2/build-script-build)
#
# It seems cross/cargo is reusing something from previous builds it shouldn't.
# the finished artifact resides in ../target/x86_64-unknown-linux-musl/release
# so deleting the temporary files in trget/releae is no problem
echo "Cleaning up...."
rm -rf ../target/release


# Buidling ARM64
echo "Building ARM64"
cross build --target aarch64-unknown-linux-musl --bin databroker-cli --release
echo "Cleaning up...."
rm -rf ../target/release


# Build RISCV64, this is a glibc based build, as musl is not
# yet supported
echo "Building RISCV64"
cross build --target riscv64gc-unknown-linux-gnu --bin databroker-cli --release
echo "Cleaning up...."
rm -rf ../target/release

# Prepare dist folders
echo "Prepare amd64 dist folder"
mkdir ../dist/amd64
cp ../target/x86_64-unknown-linux-musl/release/databroker-cli ../dist/amd64
cp -r ./databroker-cli/thirdparty ../dist/amd64

echo "Prepare arm64 dist folder"
mkdir ../dist/arm64
cp ../target/aarch64-unknown-linux-musl/release/databroker-cli ../dist/arm64
cp -r ./databroker-cli/thirdparty ../dist/arm64

echo "Prepare riscv64 dist folder"
mkdir ../dist/riscv64
cp ../target/riscv64gc-unknown-linux-gnu/release/databroker-cli ../dist/riscv64
cp -r ./databroker-cli/thirdparty ../dist/riscv64
