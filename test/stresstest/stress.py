#!/usr/bin/python3

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################

import time

import stressClient

vissClient = stressClient.StressClient()
count=0

print("VSS Server       : {}".format(vissClient.cfg['ip']+':'+vissClient.cfg['port']))
print("JWT token file   : {}".format(vissClient.cfg['token']))
print("Timeout [s]      : {}".format(vissClient.cfg['timeout']))

while True:
    vissClient.commThread.setValue("Vehicle.OBD.EngineLoad", (count%110)+1,timeout=vissClient.cfg['timeout'])
    vissClient.commThread.setValue("Vehicle.Speed", count,timeout=vissClient.cfg['timeout'])
    vissClient.commThread.setValue("Vehicle.Cabin.Door.Row1.Right.Shade.Switch", "Open",timeout=vissClient.cfg['timeout'])
    
    time.sleep(vissClient.cfg['timeout'])
