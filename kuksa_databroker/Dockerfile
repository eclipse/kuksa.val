# /********************************************************************************
# * Copyright (c) 2022, 2023 Contributors to the Eclipse Foundation
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

FROM messense/rust-musl-cross:x86_64-musl AS builder-amd64
ENV BUILDTARGET="x86_64-unknown-linux-musl"


FROM messense/rust-musl-cross:aarch64-musl AS builder-arm64
ENV BUILDTARGET="aarch64-unknown-linux-musl"

FROM builder-$TARGETARCH as builder
ARG TARGETARCH

RUN apt-get update && apt-get install -y protobuf-compiler \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# This will speed up fetching the crate.io index in the future, see
# https://blog.rust-lang.org/2022/06/22/sparse-registry-testing.html
ENV CARGO_UNSTABLE_SPARSE_REGISTRY=true

RUN echo "Building for $TARGETARCH"
ENV SRC="/home/rust/src"
WORKDIR ${SRC}

ENV CARGO_NET_GIT_FETCH_WITH_CLI=true

RUN cargo install cargo-license

# We need to add all files, because .git directory is needed to
# extract verson information during build, and we can not leave
# files out, if we do not want -dirty tags on every build
# Does not influence final image size due to multistage build
COPY . .

# Creating BOM
WORKDIR ${SRC}/kuksa_databroker/createbom
RUN rm -rf ../thirdparty
RUN python3 createbom.py ../databroker

WORKDIR ${SRC}/kuksa_databroker/databroker 

RUN cargo build --bin databroker --release --target $BUILDTARGET
RUN mv ${SRC}/target/$BUILDTARGET/release/databroker ${SRC}/

FROM scratch

COPY --from=builder /home/rust/src/databroker /app/databroker
COPY --from=builder /home/rust/src/kuksa_databroker/databroker/thirdparty /app/thirdparty

ADD ./data/vss-core/vss_release_3.1.1.json vss_release_3.1.1.json

ENV KUKSA_DATA_BROKER_ADDR=0.0.0.0
ENV KUKSA_DATA_BROKER_PORT=55555
ENV KUKSA_DATA_BROKER_METADATA_FILE=vss_release_3.1.1.json

EXPOSE $KUKSA_DATA_BROKER_PORT

ENTRYPOINT [ "/app/databroker" ]
