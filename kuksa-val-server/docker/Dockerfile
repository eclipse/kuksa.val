# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

FROM python:3.10-slim-bullseye as build

LABEL maintainer="Sebastian Schildt <sebastian.schildt@de.bosch.com>"



RUN apt-get update -qqy && apt-get upgrade -qqy && apt-get install -qqy git cmake build-essential libssl-dev libmosquitto-dev libboost-all-dev


WORKDIR /
# Build and install grpc
ENV GRPC_VER=1.44.0
RUN git clone --recurse-submodules -b v${GRPC_VER}  --single-branch --depth 1 --shallow-submodules https://github.com/grpc/grpc
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

ADD . /kuksa.val
RUN  rm -rf /kuksa.val/kuksa-val-server/build  && mkdir  /kuksa.val/kuksa-val-server/build 
WORKDIR /kuksa.val/kuksa-val-server/build
RUN cmake ..
RUN make -j 4
RUN ls
RUN /kuksa.val/kuksa-val-server/docker/collect-deployment-artifacts.sh


FROM python:3.10-slim-bullseye

COPY --from=build /deploy /kuksa.val
WORKDIR /kuksa.val

ENV LOG_LEVEL=INFO
#Usually you want this to be 0.0.0.0 when using porter port expose via -p. 
ENV BIND_ADDRESS=0.0.0.0

EXPOSE 8090/tcp

CMD /kuksa.val/startkuksaval.sh


