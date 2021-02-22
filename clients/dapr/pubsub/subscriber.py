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

from cloudevents.sdk.event import v1
from dapr.ext.grpc import App

import os, sys, json, configparser

app = App()

@app.subscribe(pubsub_name='pubsub', topic='Vehicle.Speed')
def mytopic(event: v1.Event) -> None:
    data = json.loads(event.Data())
    print(f'Subscriber received: value="{data["value"]}", timestamp = "{data["timestamp"]}", topic="{data["topic"]}", content_type="{event.content_type}"',flush=True)

@app.subscribe(pubsub_name='pubsub', topic='Vehicle.OBD.Speed')
def mytopic(event: v1.Event) -> None:
    data = json.loads(event.Data())
    print(f'Subscriber received: value="{data["value"]}", timestamp = "{data["timestamp"]}", topic="{data["topic"]}", content_type="{event.content_type}"',flush=True)

app.run(50051)
