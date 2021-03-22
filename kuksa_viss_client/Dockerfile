# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# Note: This dockerfile needs to be executed one level above in the root folder

FROM python:3.8-alpine as build
RUN apk update && apk add git
ADD . /kuksa.val
WORKDIR /kuksa.val
RUN pip install --upgrade pip build
RUN rm -rf dist && python -m build
RUN mkdir /kuksa_viss_client
RUN pip install --target /kuksa_viss_client --no-cache-dir dist/*.whl 

FROM python:3.8-alpine

COPY --from=build /kuksa_viss_client /kuksa_viss_client
ENV PYTHONUNBUFFERED=yes
WORKDIR /kuksa_viss_client
CMD ["/usr/local/bin/python", "-m", "kuksa_viss_client"]
