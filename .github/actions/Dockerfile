# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
# Quick docker for doing development

FROM ubuntu:20.04 as build

RUN apt-get  update && DEBIAN_FRONTEND="noninteractive" apt-get -y install libssl-dev  \
                        pkg-config \
                        build-essential \
                        gnupg2 \ 
                        wget \ 
                        software-properties-common \
                        git \
                        cmake \
                        libmosquitto-dev \
                        gcovr

# Build and install Boost 1.75
ENV BOOST_VER=1.75.0
ENV BOOST_VER_=1_75_0
RUN wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VER}/source/boost_${BOOST_VER_}.tar.bz2
RUN tar xjf boost_${BOOST_VER_}.tar.bz2
WORKDIR /boost_${BOOST_VER_}
RUN ./bootstrap.sh
RUN ./b2 -j 6 install

WORKDIR /
# Build and install grpc
ENV GRPC_VER=1.40.0
RUN git clone --recurse-submodules -b v${GRPC_VER} https://github.com/grpc/grpc
RUN mkdir -p /grpc/build
WORKDIR /grpc/build
RUN cmake -DgRPC_INSTALL=ON  -DgRPC_BUILD_TESTS=OFF -DgRPC_SSL_PROVIDER=package ..
RUN make -j 6
RUN make install
RUN mkdir -p /grpc/third_party/abseil-cpp/build
WORKDIR /grpc/third_party/abseil-cpp/build
RUN cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
RUN make -j 6
RUN make install

COPY run_tests.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]