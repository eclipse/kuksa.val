#!/usr/bin/python3

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
########################################################################

import time

import stressClient

vissClient = stressClient.StressClient()
count = 0

print("VSS Server       : {}".format(vissClient.commThread.backend.serverIP + ':' +
                                     str(vissClient.commThread.backend.serverPort)))
print("JWT token file   : {}".format(vissClient.commThread.backend.token_or_tokenfile))
print("Timeout [s]      : {}".format(vissClient.commThread.timeout))

while True:
    vissClient.commThread.setValue("Vehicle.OBD.EngineLoad", str((count % 110)+1),
                                   timeout=vissClient.commThread.timeout)
    vissClient.commThread.setValue("Vehicle.Speed", str(count), timeout=vissClient.commThread.timeout)
    vissClient.commThread.setValue("Vehicle.Cabin.Door.Row1.Right.Shade.Switch", "Open",
                                   timeout=vissClient.commThread.timeout)
    vissClient.commThread.getValue("Vehicle.Speed", vissClient.commThread.timeout)

    time.sleep(vissClient.commThread.timeout)
