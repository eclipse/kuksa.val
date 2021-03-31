# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

FROM python:3.8-alpine
RUN apk --no-cache update && apk  --no-cache add gpsd
ADD . /kuksa_gps_feeder
WORKDIR /kuksa_gps_feeder
RUN pip install --no-cache-dir -r requirements.txt 
ENV PYTHONUNBUFFERED=yes

ENV GPSD_SOURCE=udp://0.0.0.0:29998
ENV GPSD_LISTENER_PORT=2948

EXPOSE 29998/udp
CMD gpsd -S ${GPSD_LISTENER_PORT} ${GPSD_SOURCE}; ./gpsd_feeder.py
