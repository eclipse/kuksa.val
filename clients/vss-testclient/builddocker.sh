#!/bin/bash
# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#

echo "Try building vss-testclient docker"
cd ../

if [ ! -d vss-testclient/tokens ]; then
    echo "Copying example token"
    mkdir vss-testclient/tokens
    cp -r ../certificates/jwt/*token vss-testclient/tokens
else
    echo "Using existing token folder"
fi

sudo docker build -f vss-testclient/Dockerfile .