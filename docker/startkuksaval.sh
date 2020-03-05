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


if [ -e /config/vss.json ]
then
    echo "Using existing vss tree"
else
    echo "No VSS tree, initilaize with example"
    cp /kuksa.val/exampleconfig/vss.json /config/
fi

mkdir -p /config/certs

if [ -e /config/certs/Server.key ]
then
    echo "Using existing server keys"
else
    echo "No server keys configured, initialize with example"
    cp /kuksa.val/exampleconfig/certs/Server.key /config/certs/
    cp /kuksa.val/exampleconfig/certs/Server.pem /config/certs/
fi


LD_LIBRARY_PATH=./ ./w3c-visserver --vss /config/vss.json --cert-path /config/certs --log-level $LOG_LEVEL --address $BIND_ADDRESS
