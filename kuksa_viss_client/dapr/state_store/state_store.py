#! /usr/bin/env python

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################


import os, sys, configparser, signal
import json
import time

from dapr.clients import DaprClient
from dapr.clients.grpc._request import TransactionalStateOperation, TransactionOperationType
from dapr.clients.grpc._state import StateItem

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../common/"))
from clientComm import VSSClientComm

class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        self.subscriptionMap = {}
        provider_config=config['kuksa_val']
        self.client = VSSClientComm(provider_config)
        self.client.start()
        self.token = provider_config.get('token', "token.json")
        self.client.authorize(self.token)
        
    def shutdown(self):
        self.client.stopComm()

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
        value = str(jsonMsg["value"])
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


