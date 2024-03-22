#!/bin/bash
#
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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

# Builds for a given target and collects data to be distirbuted in docker. Needs
# Rust target triplett (i.e. x86_64-unknown-linux-musl) and the corresponding docker
# architecture (i.e. amd64) as input
function build_target() {
    target_rust=$1
    target_docker=$2

    echo "Building databroker-cli for target $target_rust"
    cross build --target $target_rust --bin databroker-cli --release

    echo "Create $target_rust SBOM"
    cargo cyclonedx -v -f json --describe binaries --spec-version 1.4 --target $target_rust --manifest-path ../Cargo.toml

    echo "Prepare $target_docker dist folder"
    mkdir ../dist/$target_docker
    cp ../target/$target_rust/release/databroker-cli ../dist/$target_docker
    cp ./databroker-cli/databroker-cli_bin.cdx.json ../dist/$target_docker/sbom.json

    rm -rf ../dist/$target_docker/thirdparty-licenses || true

    cd createbom/
    rm -rf ../databroker/thirdparty-licenses || true
    python3 collectlicensefromcyclonedx.py ../databroker-cli/databroker-cli_bin.cdx.json ../../dist/$target_docker/thirdparty-licenses --curation ../licensecuration.yaml
    cd ..

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
    # so deleting the temporary files in target/releae is no problem
    echo "Cleaning up...."
    rm -rf ../target/release

}

# Starting a fresh build
echo "Cargo clean, to start fresh..."
cargo clean
rm -rf ../dist || true
mkdir ../dist

# Building AMD46
build_target x86_64-unknown-linux-musl amd64

# Building ARM64
build_target  aarch64-unknown-linux-musl arm64

# Build RISCV64, this is a glibc based build, as musl is not
# yet supported
# Building RISCV64
build_target  riscv64gc-unknown-linux-gnu riscv64
