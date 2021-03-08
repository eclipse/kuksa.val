# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# Note: This dockerfile needs to be executed one level above in the root folder

FROM python:3.8-alpine as build
# TODO build and upload
RUN apk update && apk add git bash
ADD . /kuksa.val
WORKDIR /kuksa.val
RUN pip install --upgrade pip build
RUN python -m build

FROM python:3.8-alpine

COPY --from=build /kuksa.val/dist /kuksa_dist
RUN pip install kuksa_dist/*.whl 
RUN rm -rf kuksa_dist
ENV PYTHONUNBUFFERED=yes
CMD ["kuksa_client"]
