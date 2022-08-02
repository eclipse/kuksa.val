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
