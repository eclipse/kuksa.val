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

while True:
    vissClient.commThread.setValue("Vehicle.OBD.EngineLoad", (count%110)-10,timeout=vissClient.cfg['timeout'])
    vissClient.commThread.setValue("Vehicle.Speed", count,timeout=vissClient.cfg['timeout'])
    vissClient.commThread.setValue("Vehicle.Cabin.Door.Row1.Right.Shade.Switch", "Open",timeout=vissClient.cfg['timeout'])
    if count < 100:
        count += 1
    else:
        count = 0
    time.sleep(vissClient.cfg['timeout'])
