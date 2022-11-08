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
FROM rust:1.63 as builder


RUN rustup component add rustfmt

RUN mkdir /build
WORKDIR /build

ADD kuksa_databroker kuksa_databroker
ADD proto proto

# Creating BOM
RUN cargo install cargo-license
WORKDIR /build/kuksa_databroker/createbom
RUN rm -rf ../thirdparty
RUN python3 createbom.py ../databroker

WORKDIR /build/kuksa_databroker/databroker

ENV RUSTFLAGS='-C link-arg=-s'
RUN cargo build --bin databroker --release



FROM  debian:buster-slim

COPY --from=builder /build/kuksa_databroker/databroker/target/release/databroker /app/databroker
COPY --from=builder /build/kuksa_databroker/databroker/thirdparty /app/thirdparty

ADD ./data/vss-core/vss_release_3.0.json vss_release_3.0.json

ENV KUKSA_DATA_BROKER_ADDR=0.0.0.0
ENV KUKSA_DATA_BROKER_PORT=55555
ENV KUKSA_DATA_BROKER_METADATA_FILE=vss_release_3.0.json

EXPOSE $KUKSA_DATA_BROKER_PORT

ENTRYPOINT [ "/app/databroker" ]
