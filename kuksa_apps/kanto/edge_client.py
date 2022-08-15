#########################################################################
# Copyright (c) 2022 Bosch.IO GmbH
# Copyright (c) 2022 Contributors to the Eclipse Foundation
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

import configparser
import signal
import sys
from os.path import exists

import paho.mqtt.client as mqtt
from ditto.client import Client
from ditto.model.feature import Feature
from ditto.protocol.things.commands import Command

from edge_device_info import EdgeDeviceInfo
from kuksa_viss_client import KuksaClientThread
from utils import process_tree, process_signal

FEATURE_ID_VSS = "VSS"

EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO = "edge/thing/response"
EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO_REQUEST = "edge/thing/request"


class EdgeClient:
    def __init__(self, config):

        if "kanto_kuksa_bridge" not in config:
            print("kanto_kuksa_bridge section missing from configuration, exiting.")
            sys.exit(-1)

        kuksa_val_address = config.get("kuksa_val", "ip", fallback="127.0.0.1")
        self.mqtt_client = None
        cfg = {"ip": kuksa_val_address}
        self.kuksa_client = KuksaClientThread(cfg)
        self.device_info = None
        self.ditto_client = None

    def on_connect(self, client: mqtt.Client, obj, flags, rc):
        print("Connected with result code " + str(rc))
        self.mqtt_client = client
        # init ditto client
        self.ditto_client = Client(paho_client=self.mqtt_client)
        self.ditto_client.connect()
        # init kuksa client
        self.kuksa_client.start()
        self.kuksa_client.authorize()
        # trigger initialization
        self.mqtt_client.subscribe(EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO)
        self.mqtt_client.publish(EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO_REQUEST, None, 1)

    def on_message(self, client, userdata, msg):
        try:
            if msg.topic == EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO:
                if self.device_info is None:
                    self.device_info = EdgeDeviceInfo()
                    self.device_info.unmarshal_json(msg.payload)
                    self.add_vss_feature()
                    self.subscribe()
                else:
                    print('device info already available - discarding message')
                return
        except Exception as ex:
            print(ex)

    def subscribe(self, config):
        # get vss paths from config
        vss_paths = [e.strip() for e in config.get('paths', 'vss_paths').split(',')]

        for path in vss_paths:
            self.kuksa_client.subscribe(path, self.on_kuksa_signal)

    def add_vss_feature(self, config):

        # add the vss feature
        cmd = Command(self.device_info.deviceId).feature(FEATURE_ID_VSS).modify(Feature().to_ditto_dict())
        cmd_envelope = cmd.envelope(response_required=False, content_type="application/json")
        self.ditto_client.send(cmd_envelope)

        # add the vss tree as properties
        vss_subtree = config.get("vss", "vss_subtree")

        if vss_subtree[-1] == "/":
            vss_subtree += "*"
        elif vss_subtree[len(vss_subtree) - 2:] != "/*":
            vss_subtree += "/*"

        vss_tree = self.kuksa_client.getValue(vss_subtree)
        processed = process_tree(vss_tree)
        for key, val in processed.items():
            if 'Heading' in key or 'Accuracy' in key:
                continue
            cmd = Command(self.device_info.deviceId).feature_property(FEATURE_ID_VSS, key).modify(val)
            cmd_envelope = cmd.envelope(response_required=False, content_type="application/json")
            self.ditto_client.send(cmd_envelope)

    def on_kuksa_signal(self, message):
        if self.device_info is None:
            print('no device info is initialized to process VSS data for')
            return
        path, val = process_signal(message)
        # update property
        cmd = Command(self.device_info.deviceId).feature_property(FEATURE_ID_VSS, path.replace('.', '/')).modify(val)
        cmd_envelope = cmd.envelope(response_required=False, content_type="application/json")
        self.ditto_client.send(cmd_envelope)

    def shutdown(self):
        self.kuksa_client.stop()
        self.ditto_client.disconnect()


if __name__ == "__main__":

    path_to_config = "config/edge_client.ini"
    file_exists = exists(path_to_config)

    if file_exists is False:
        print("No configuration file found at: " + path_to_config + ". Exiting.")
        sys.exit(-1)

    conf = configparser.ConfigParser()
    conf.read(path_to_config)

    paho_client = mqtt.Client()
    edge_client = EdgeClient(conf)

    paho_client.on_connect = edge_client.on_connect
    paho_client.on_message = edge_client.on_message

    paho_client.connect(conf.get("paho_client", "ip", fallback="172.0.0.1"))


    def termination_signal_received(signal_number, frame):
        print("Received termination signal. Shutting down")
        edge_client.shutdown()
        paho_client.disconnect()


    signal.signal(signal.SIGINT, termination_signal_received)
    signal.signal(signal.SIGQUIT, termination_signal_received)
    signal.signal(signal.SIGTERM, termination_signal_received)
    print("before loop forever")
    paho_client.loop_forever()
