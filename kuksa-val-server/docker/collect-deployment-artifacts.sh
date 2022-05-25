#!/bin/sh
#
# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#
# Helper script to collect executables, config and libraries needed for minimal docker

mkdir /deploy
cp /kuksa.val/kuksa-val-server/build/src/kuksa-val-server /deploy

cp /kuksa.val/docker/startkuksaval.sh /deploy

ldd /kuksa.val/kuksa-val-server/build/src/kuksa-val-server | grep "=>" | awk '{print $3}' |   xargs -I '{}' cp -v '{}' /deploy

mkdir /deploy/exampleconfig
mkdir /deploy/exampleconfig/certs

cp /kuksa.val/kuksa-val-server/build/src/config.ini  /deploy/exampleconfig/config.ini
cp /kuksa.val/kuksa-val-server/build/src/vss_release_2.2.json  /deploy/exampleconfig/vss.json
cp -r  /kuksa.val/kuksa_certificates/* /deploy/exampleconfig/certs/
