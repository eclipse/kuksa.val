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

print("VSS Server       : {}".format(vissClient.commThread.serverIP+':'+str(vissClient.commThread.serverPort)))
print("JWT token file   : {}".format(vissClient.commThread.tokenfile))
print("Timeout [s]      : {}".format(vissClient.commThread.timeout))

while True:
    vissClient.commThread.setValue("Vehicle.OBD.EngineLoad", (count%110)+1,timeout=vissClient.commThread.timeout)
    vissClient.commThread.setValue("Vehicle.Speed", count,timeout=vissClient.commThread.timeout)
    vissClient.commThread.setValue("Vehicle.Cabin.Door.Row1.Right.Shade.Switch", "Open",timeout=vissClient.commThread.timeout)
    vissClient.commThread.getValue("Vehicle.Speed",vissClient.commThread.timeout)
    
    time.sleep(vissClient.commThread.timeout)
