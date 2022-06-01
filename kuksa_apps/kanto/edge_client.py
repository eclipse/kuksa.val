# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
#
# SPDX-License-Identifier: EPL-2.0

import signal
import sys

import paho.mqtt.client as mqtt
from ditto.client import Client
from ditto.model.feature import Feature
from ditto.protocol.things.commands import Command
from kuksa_viss_client import KuksaClientThread

from edge_device_info import EdgeDeviceInfo
from utils import process_tree, process_signal

FEATURE_ID_VSS = "VSS"

EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO = "edge/thing/response"
EDGE_CLOUD_CONNECTOR_TOPIC_DEV_INFO_REQUEST = "edge/thing/request"


class EdgeClient:
    def __init__(self):
        self.mqtt_client = None
        cfg = {"ip": "127.0.0.1"}
        self.kuksa_client = KuksaClientThread(cfg)
        self.device_info = None
        self.ditto_client = None

    def on_connect(self, client:mqtt.Client, obj, flags, rc):
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

    def subscribe(self):
        self.kuksa_client.subscribe('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Altitude', self.on_kuksa_signal)
        self.kuksa_client.subscribe('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Latitude', self.on_kuksa_signal)
        self.kuksa_client.subscribe('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Longitude', self.on_kuksa_signal)
        # self.kuksa_client.subscribe('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Accuracy', self.on_kuksa_signal)
        self.kuksa_client.subscribe('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Speed', self.on_kuksa_signal)

    def add_vss_feature(self):
        # add the vss feature
        cmd = Command(self.device_info.deviceId).feature(FEATURE_ID_VSS).modify(Feature().to_ditto_dict())
        cmd_envelope = cmd.envelope(response_required=False, content_type="application/json")
        self.ditto_client.send(cmd_envelope)

        # add the vss tree as properties
        vss_tree = self.kuksa_client.getValue('Vehicle/Cabin/Infotainment/Navigation/CurrentLocation/*')
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
        cmd = Command(self.device_info.deviceId).feature_property(FEATURE_ID_VSS, path.replace('.','/')).modify(val)
        cmd_envelope = cmd.envelope(response_required=False, content_type="application/json")
        self.ditto_client.send(cmd_envelope)

    def shutdown(self):
        self.kuksa_client.stop()
        self.ditto_client.disconnect()


if __name__ == "__main__":
    paho_client = mqtt.Client()
    edge_client = EdgeClient()

    paho_client.on_connect = edge_client.on_connect
    paho_client.on_message = edge_client.on_message
    paho_client.connect("localhost")


    def termination_signal_received(signal_number, frame):
        print("Received termination signal. Shutting down")
        edge_client.shutdown()
        paho_client.disconnect()


    signal.signal(signal.SIGINT, termination_signal_received)
    signal.signal(signal.SIGQUIT, termination_signal_received)
    signal.signal(signal.SIGTERM, termination_signal_received)
    print('before loop forever')
    paho_client.loop_forever()
