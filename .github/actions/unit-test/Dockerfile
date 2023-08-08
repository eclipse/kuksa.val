# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
# Quick docker for doing development

FROM ubuntu:20.04 as build

RUN apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get -y install libssl-dev  \
                        pkg-config \
                        build-essential \
                        gnupg2 \
                        wget \
                        software-properties-common \
                        git \
                        cmake \
                        libmosquitto-dev \
                        gcovr lcov

COPY run_tests.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
