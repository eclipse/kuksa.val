#!/usr/bin/env bash

########################################################################
# Copyright (c) 2018-2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################

VERSION="0.1.1"
NAME="kuksa-val"

if [ -z "$VERSION" ]; then
    echo "Failed to extract version string" 1>&2
    exit 1
fi

EXPERIMENTAL=`sudo docker version | grep Experimental | tail -n 1 | xargs`
if [ "$EXPERIMENTAL" != "Experimental: true" ]; then
    echo "Please enable the docker engine's experimental features" 1>&2
    exit 1
fi

function build {
    ARCH=$1

    # generate Dockerfile.build
    case ${ARCH} in
    'amd64')
        sed -e "s/arm64v8/amd64/g" Dockerfile > Dockerfile.build
        sed -i -e "s/^.*qemu-aarch64-static.*$//g" Dockerfile.build
        ;;
    'arm64')
        cp /usr/bin/qemu-aarch64-static ./
        if [ "$?" != "0" ]; then
            echo "Please install the qemu-user-static package" 1>&2
            exit 1
        fi
        cp Dockerfile Dockerfile.build
        ;;
     'arm32v7')
        cp /usr/bin/qemu-arm-static ./
        if [ "$?" != "0" ]; then
            echo "Please install the qemu-user-static package" 1>&2
            exit 1
        fi
        sed -e "s/arm64v8/arm32v7/g" Dockerfile > Dockerfile.build
        sed -i -e "s/qemu-aarch64-static/qemu-arm-static/g" Dockerfile.build
        ;;
    'arm32v6')
        cp /usr/bin/qemu-arm-static ./
        if [ "$?" != "0" ]; then
            echo "Please install the qemu-user-static package" 1>&2
            exit 1
        fi
        sed -e "s/arm64v8/arm32v6/g" Dockerfile > Dockerfile.build
        sed -i -e "s/qemu-aarch64-static/qemu-arm-static/g" Dockerfile.build
        ;;
    *)
        echo "Unsupported architecture: $ARCH" 1>&2
        exit 1
        ;;
    esac

    # build image
    cd ../
    sudo docker build --squash --platform linux/$ARCH -f ./docker/Dockerfile.build -t ${ARCH}/${NAME}:${VERSION} .
    if [ "$?" != "0" ]; then
        echo "Docker build failed."
        exit 1
    fi
    cd docker

    # cleanup
    rm -f Dockerfile.build
    rm -f qemu-aarch64-static
    rm -f qemu-arm-static
}

MYDIR=$(dirname "$0")
cd $MYDIR

ARCHS="$@"
if [ -z "$ARCHS" ]; then
    ARCHS="amd64 arm64 arm32v6"
fi

for ARCH in $ARCHS; do
    build $ARCH
done
