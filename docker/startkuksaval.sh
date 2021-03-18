#!/bin/sh
# 
# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#
# Script for starting kuksa.val inside docker

echo "Starting kuksa.val"

mkdir -p /config/certs

if [ -e /config/vss.json ]
then
    echo "Using existing vss tree"
else
    echo "No VSS tree, initilaize with example"
    cp /kuksa.val/exampleconfig/vss.json /config/
fi


if [ -e /config/certs/Server.key ]
then
    echo "Using existing server keys"
else
    echo "No server keys configured, initialize with example"
    cp /kuksa.val/exampleconfig/certs/Server.key /config/certs/
    cp /kuksa.val/exampleconfig/certs/Server.pem /config/certs/
fi


if [ -e /config/certs/jwt.key.pub ]
then
    echo "Using existing jwt key"
else
    echo "No jwt key configured, initialize with example"
    cp /kuksa.val/exampleconfig/certs/jwt/jwt.key.pub /config/certs/
fi

if [[ -z "${KUKSAVAL_OPTARGS}" ]]; then
  OPTARGS=""
else
  OPTARGS="${KUKSAVAL_OPTARGS}"
fi


LD_LIBRARY_PATH=./ ./kuksa-val-server --vss /config/vss.json --cert-path /config/certs --log-level $LOG_LEVEL --address $BIND_ADDRESS $OPTARGS
