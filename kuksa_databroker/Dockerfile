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

FROM --platform=$BUILDPLATFORM alpine:latest as builder

ARG TARGETPLATFORM

WORKDIR /workdir

COPY ./databroker* /workdir

RUN if [ "$TARGETPLATFORM" = "linux/amd64" ] ; \
    then tar -xzf databroker_x86_64.tar.gz && cp -v ./target/release/databroker ./databroker; \
    else tar -xzf databroker_aarch64.tar.gz && cp -v ./target/aarch64-unknown-linux-gnu/release/databroker ./databroker; fi

FROM --platform=$TARGETPLATFORM debian:buster-slim

COPY --from=builder workdir/databroker /app/databroker

ENV KUKSA_DATA_BROKER_ADDR=0.0.0.0
ENV KUKSA_DATA_BROKER_PORT=55555

EXPOSE $KUKSA_DATA_BROKER_PORT

ENTRYPOINT [ "/app/databroker" ]
