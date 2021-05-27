# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

FROM python:3.8-alpine
ADD . /kuksa_dbc_feeder
WORKDIR /kuksa_dbc_feeder
RUN pip install --no-cache-dir -r requirements.txt 
ENV PYTHONUNBUFFERED=yes

CMD ./dbcfeeder.py
