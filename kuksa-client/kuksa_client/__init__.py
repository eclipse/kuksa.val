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

import asyncio
import json
import os
import queue
import ssl
import sys
import threading
import time
from typing import Any
from typing import Dict
from typing import Iterable
import uuid

from kuksa_client._metadata import *

from . import cli_backend


class KuksaClientThread(threading.Thread):

    # Constructor
    def __init__(self, config):
        super().__init__()

        self.backend = cli_backend.Backend.from_config(config)
        self.loop = None

    def checkConnection(self):
        return self.backend.checkConnection()

    def stop(self):
        self.backend.stop()

    # Do authorization by passing a jwt token or a token file
    def authorize(self, token=None, timeout=5):
        return self.backend.authorize(token, timeout)

    # Update VSS Tree Entry
    def updateVSSTree(self, jsonStr, timeout=5):
        return self.backend.updateVSSTree(jsonStr, timeout)

    # Update Meta Data of a given path
    def updateMetaData(self, path, jsonStr, timeout=5):
        return self.backend.updateMetaData(path, jsonStr, timeout)

    # Get Meta Data of a given path
    def getMetaData(self, path, timeout=5):
        return self.backend.getMetaData(path, timeout)

    # Set value to a given path
    def setValue(self, path, value, attribute="value", timeout=5):
        return self.backend.setValue(path, value, attribute, timeout)

    # Set value of every given path
    def setValues(self, updates: Dict[str, Any], attribute="value", timeout=5):
        return self.backend.setValues(updates, attribute, timeout)

    # Get value to a given path
    def getValue(self, path, attribute="value", timeout=5):
        return self.backend.getValue(path, attribute, timeout)

    # Get value of every given path
    def getValues(self, paths: Iterable[str], attribute="value", timeout=5):
        return self.backend.getValues(paths, attribute, timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, attribute="value", timeout=5):
        return self.backend.subscribe(path, callback, attribute, timeout)

    def subscribeMultiple(self, paths, callback, attribute="value", timeout=5):
        return self.backend.subscribeMultiple(paths, callback, attribute, timeout)

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, sub_id, timeout=5):
        return self.backend.unsubscribe(sub_id, timeout)

    def disconnect(self, timeout=5):
        return self.backend.disconnect(timeout)

    def connect(self, timeout=5):
        return self.backend.connect(timeout)

    # Thread function: Start the asyncio loop
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.backend.mainLoop())
