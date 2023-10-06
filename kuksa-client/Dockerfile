# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# Note: This dockerfile needs to be executed one level above in the root folder


FROM python:3.10-slim-bookworm as build
# binutils is required by pyinstaller to strip any .so libs that are collected
# git is used to determine & embed version information during build time
RUN apt update && apt -yy install binutils git
RUN pip install --upgrade pip build pyinstaller

# We must copy the whole repository otherwise version lookup by tag would not work
COPY . /kuksa.val/
WORKDIR /kuksa.val/kuksa-client

RUN python3 -m build
# We install globally on build container, so pyinstaller can easily gather all files
RUN pip install --no-cache-dir dist/*.whl

WORKDIR /
RUN rm -rf dist
# Letting pyinstaller collect everything that is required
RUN pyinstaller --collect-data kuksa_certificates  --collect-data kuksa_client --clean  -s  /usr/local/bin/kuksa-client


# Debian 12 is bookworm, so the glibc version matches. Distroless is a lot smaller than
# Debian slim versions
# For development add :debug like this
# FROM gcr.io/distroless/base-debian12:debug  to get a busybox shell as well
FROM gcr.io/distroless/base-debian12

COPY --from=build /dist/kuksa-client /kuksa-client

# pyinstaller doesn't pick up transient libz dependency, so copying it manually
COPY --from=build /usr/lib/*-linux-gnu/libz.so.1 /lib/
# stty is required by cmd2
COPY --from=build /usr/bin/stty /bin/

ENV PYTHONUNBUFFERED=yes

WORKDIR /kuksa-client
ENTRYPOINT ["/kuksa-client/kuksa-client"]
