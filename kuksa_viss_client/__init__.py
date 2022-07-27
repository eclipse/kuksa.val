#! /usr/bin/env python

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

import os, sys, threading, queue, ssl, json, time 
import uuid
import asyncio
scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, ".."))
from kuksa_viss_client._metadata import *
from .KuksaWsComm import KuksaWsComm
from .KuksaGrpcComm import KuksaGrpcComm

class KuksaClientThread(threading.Thread):

    # Constructor
    def __init__(self, config):
        super(KuksaClientThread, self).__init__()
        
        self.serverProtocol = config.get("protocol", "ws")
        self.KuksaClientObject = None
        if self.serverProtocol == "ws":
            self.KuksaClientObject = KuksaWsComm(config)
        elif self.serverProtocol == "grpc":
            self.KuksaClientObject = KuksaGrpcComm(config)

    def checkConnection(self):
        return self.KuksaClientObject.checkConnection()

    def stop(self):
        self.KuksaClientObject.stop() 

    # Do authorization by passing a jwt token or a token file
    def authorize(self, token=None, timeout = 2):
        return self.KuksaClientObject.authorize(token)

    # Update VSS Tree Entry 
    def updateVSSTree(self, jsonStr, timeout = 5):
        return self.KuksaClientObject.updateVSSTree(jsonStr, timeout)

    # Update Meta Data of a given path
    def updateMetaData(self, path, jsonStr, timeout = 5):
        return self.KuksaClientObject.updateMetaData(path, jsonStr, timeout)

    # Get Meta Data of a given path
    def getMetaData(self, path, timeout = 1):
        return self.KuksaClientObject.getMetaData(path, timeout)

    # Set value to a given path
    def setValue(self, path, value, attribute="value", timeout = 1):
        return self.KuksaClientObject.setValue(path, value, attribute, timeout)

    # Get value to a given path
    def getValue(self, path, attribute="value", timeout = 5):
        return self.KuksaClientObject.getValue(path, attribute, timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, attribute = "value", timeout = 5):
        return self.KuksaClientObject.subscribe(path, callback, attribute, timeout)

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, id, timeout = 5):
        return self.KuksaClientObject.unsubscribe(id, timeout)

    # Thread function: Start the asyncio loop
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.KuksaClientObject.mainLoop())
