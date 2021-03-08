# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

# This dockerfile needs to be executed one lelvel above to have access to common
# and it expects a token folder (where you can preload some security tokens)
# see build.sh for an example how to run this

FROM python:3.8-alpine
# TODO build and upload
ADD . /kuksa.val
WORKDIR /kuksa.val
RUN ls
RUN pip install --upgrade build
RUN python -m build
RUN pip install kuksa-client

ENV PYTHONUNBUFFERED=yes
CMD ["kuksa-client"]
