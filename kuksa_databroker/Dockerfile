# /********************************************************************************
# * Copyright (c) 2022 Contributors to the Eclipse Foundation
# *
# * See the NOTICE file(s) distributed with this work for additional
# * information regarding copyright ownership.
# *
# * This program and the accompanying materials are made available under the
# * terms of the Apache License 2.0 which is available at
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * SPDX-License-Identifier: Apache-2.0
# ********************************************************************************/

# This is expected to be executed in the kuksa.val top-level directory

# This builds KUKSA databroker

FROM rust:1.65-alpine as base
RUN apk add git musl-dev python3 protoc ncurses-terminfo-base make 
RUN rustup component add rustfmt


FROM base AS builder-amd64
RUN echo "AMD64 quirks"
# it seems on AMD build some dependency requires zlib headers
# also the grpc proto compilation always falls back to
# some binaries downloaded by cargo that require glibc
RUN apk add zlib-dev libc6-compat  libstdc++ gcompat


FROM base AS builder-arm64
RUN echo "ARM64 quirks"
# Need flag for MUSL arm builds, to prevent linker error
# on missing __getauxval symbol, see also
# https://github.com/rust-lang/rust/issues/89626
ENV CFLAGS=-mno-outline-atomics



FROM builder-$TARGETARCH as builder
ARG TARGETARCH
# This will speed up fetching the crate.io index in the future, see
# https://blog.rust-lang.org/2022/06/22/sparse-registry-testing.html
ENV CARGO_UNSTABLE_SPARSE_REGISTRY=true

RUN echo "Building for $TARGETARCH"

RUN mkdir /build
WORKDIR /build

# See also https://github.com/rust-lang/cargo/issues/10583, 
# without this needs more than 12GB in runner if somebody wants to buildx
# it with the common qemu setup
RUN ln -s /usr/bin/git /usr/local/cargo/bin/
ENV CARGO_NET_GIT_FETCH_WITH_CLI=true

RUN cargo install cargo-license

ADD kuksa_databroker kuksa_databroker
ADD proto proto

# Creating BOM
WORKDIR /build/kuksa_databroker/createbom
RUN rm -rf ../thirdparty
RUN python3 createbom.py ../databroker

WORKDIR /build/kuksa_databroker/databroker

ENV RUSTFLAGS='-C link-arg=-s'

RUN cargo build --bin databroker --release


FROM  scratch

COPY --from=builder /build/kuksa_databroker/databroker/target/release/databroker /app/databroker
COPY --from=builder /build/kuksa_databroker/databroker/thirdparty /app/thirdparty

ADD ./data/vss-core/vss_release_3.0.json vss_release_3.0.json

ENV KUKSA_DATA_BROKER_ADDR=0.0.0.0
ENV KUKSA_DATA_BROKER_PORT=55555
ENV KUKSA_DATA_BROKER_METADATA_FILE=vss_release_3.0.json

EXPOSE $KUKSA_DATA_BROKER_PORT

ENTRYPOINT [ "/app/databroker" ]
