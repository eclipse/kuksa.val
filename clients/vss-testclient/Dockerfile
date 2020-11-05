# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# This dockerfile needs to be executed one lelvel above to have access to common
# and it expects a topken folder (where you can preload some security tokens)
# see build.sh for an example how to run this

FROM python:3.8-alpine AS build-env


RUN mkdir /app
RUN mkdir /app/vssclient
COPY vss-testclient/testclient.py /app/vssclient/
ADD vss-testclient/tokens /app/vssclient/tokens
ADD common /app/common
WORKDIR /app/vssclient
RUN pip install --target=./ --no-cache-dir websockets cmd2 pygments


FROM python:3.8-alpine
COPY --from=build-env /app /app

ENV PYTHONUNBUFFERED=yes
WORKDIR /app/vssclient
CMD ["/usr/local/bin/python", "testclient.py"]