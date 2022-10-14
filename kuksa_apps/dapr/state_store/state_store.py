#! /usr/bin/env python3

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

import os, sys, configparser, signal
import json
import time

from dapr.clients import DaprClient
from dapr.clients.grpc._request import TransactionalStateOperation, TransactionOperationType
from dapr.clients.grpc._state import StateItem

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../../"))
from kuksa_client import KuksaClientThread

class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        self.subscriptionMap = {}
        provider_config=config['kuksa_val']
        self.client = KuksaClientThread(provider_config)
        self.client.start()
        self.client.authorize()
        
    def shutdown(self):
        self.client.stop()

    def subscribe(self, path, callback):
        print("subscribe " + path)
        res = self.client.subscribe(path, callback)
        res = json.loads(res)
        if "subscriptionId" in res:
            self.subscriptionMap[res["subscriptionId"]] = path
        print(res)

class Dapr_Storage():
    def __init__(self, config, producer):
        print("Init dapr Storage...")
        if "dapr" not in config:
            print("dapr section missing from configuration, exiting")
            sys.exit(-1)
        
        self.producer = producer
        dapr_config=config['dapr']
        if "topics" not in dapr_config:
            print("no topics sepcified, exiting")
            sys.exit(-1)
        self.topics=dapr_config.get('topics').replace(" ", "").split(',')
        self.daprClient = DaprClient()
        for topic in self.topics:
            self.storeValue(topic, self.producer.client.getValue(topic))
            self.producer.subscribe(topic, self.store)


    def storeValue(self, path, message):
        storeName = 'statestore'
        jsonMsg = json.loads(message) 
        key = path
        value = str(jsonMsg["data"]["dp"]["value"])
        self.daprClient.save_state(store_name=storeName, key=key, value=value)
        print(f"State store has successfully saved {key}: {value}")
        
        state = self.daprClient.get_state(store_name=storeName, key=key, state_metadata={"metakey": "metavalue"})
        print(f"Got value={state.data} eTag={state.etag}")

    def store(self, message):
        jsonMsg = json.loads(message) 
        key = self.producer.subscriptionMap[jsonMsg["subscriptionId"]]
        self.storeValue(key, message)


    def shutdown(self):
        self.producer.shutdown()

        
if __name__ == "__main__":
    config_candidates=['config.ini']
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile=candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)
    
    client = Dapr_Storage(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)


