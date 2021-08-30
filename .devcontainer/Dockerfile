# Copyright Robert Bosch GmbH, 2021. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.191.0/containers/ubuntu/.devcontainer/base.Dockerfile

# [Choice] Ubuntu version: bionic, focal
ARG VARIANT="focal"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

# Install additional OS packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install  libssl-dev  \
                            pkg-config \
                            build-essential \
                            gnupg2 \ 
                            wget \ 
                            cmake \
                            libmosquitto-dev \
                            gcovr \
                            python3-pip


