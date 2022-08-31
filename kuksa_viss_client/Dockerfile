# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# Note: This dockerfile needs to be executed one level above in the root folder

FROM python:3.10-alpine as build
RUN apk update && apk add git alpine-sdk linux-headers
ADD . /kuksa.val
WORKDIR /kuksa.val/kuksa_viss_client
RUN pip install --upgrade pip build grpcio grpcio-tools
RUN rm -rf dist 
RUN python3 setup.py bdist_wheel
RUN mkdir /kuksa_viss_client
RUN pip install --target /kuksa_viss_client --no-cache-dir dist/*.whl 

FROM python:3.10-alpine

RUN apk add --no-cache libstdc++
COPY --from=build /kuksa_viss_client /kuksa_viss_client
ENV PYTHONUNBUFFERED=yes
ENV GRPC_ENABLE_FORK_SUPPORT=false
WORKDIR /kuksa_viss_client
CMD ["/usr/local/bin/python", "-m", "kuksa_viss_client"]
