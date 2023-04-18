########################################################################
# Copyright (c) 2022 Robert Bosch GmbH
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
import pathlib
import queue
from typing import Any
from typing import Callable
from typing import Dict
from typing import Iterable
import uuid

from kuksa_client import cli_backend
import kuksa_client.grpc
import kuksa_client.grpc.aio
from kuksa_client.grpc import EntryUpdate


def callback_wrapper(callback: Callable[[str], None]) -> Callable[[Iterable[EntryUpdate]], None]:
    def wrapper(updates: Iterable[EntryUpdate]) -> None:
        callback(json.dumps([update.to_dict() for update in updates]))
    return wrapper


class Backend(cli_backend.Backend):
    def __init__(self, config):
        super().__init__(config)
        self.cacertificate = pathlib.Path(self.cacertificate)
        self.keyfile = pathlib.Path(self.keyfile)
        self.certificate = pathlib.Path(self.certificate)
        if self.tokenfile is not None:
            self.tokenfile = pathlib.Path(self.tokenfile)
            self.token = self.tokenfile.expanduser(
            ).read_text(encoding='utf-8').rstrip('\n')
        else:
            self.token = ""
        self.grpcConnected = False

        self.sendMsgQueue = queue.Queue()
        self.run = False

        self.AttrDict = {
            "value": (kuksa_client.grpc.Field.VALUE, kuksa_client.grpc.View.CURRENT_VALUE),
            "targetValue": (kuksa_client.grpc.Field.ACTUATOR_TARGET, kuksa_client.grpc.View.TARGET_VALUE),
            "metadata": (kuksa_client.grpc.Field.METADATA, kuksa_client.grpc.View.METADATA),
        }

    # Function to check connection status
    def checkConnection(self):
        if self.grpcConnected:
            return True
        return False

    # Function to stop the communication
    def stop(self):
        self.disconnect()
        self.run = False
        print("gRPC channel disconnected.")

    # Function to implement fetching of metadata
    def getMetaData(self, path, timeout=5):
        return self.getValue(path, "metadata", timeout)

    def updateMetaData(self, path, jsonStr, timeout=5):
        return self.setValue(path, jsonStr, "metadata", timeout)

    # Function to implement get
    def getValue(self, path, attribute="value", timeout=5):
        return self.getValues((path,), attribute, timeout)

    def getValues(self, paths, attribute="value", timeout=5):
        if attribute in self.AttrDict:
            field, view = self.AttrDict[attribute]
            entries = [kuksa_client.grpc.EntryRequest(
                path=path, view=view, fields=(field,)) for path in paths]
            requestArgs = {'entries': entries}
            return self._sendReceiveMsg(("get", requestArgs), timeout)

        return json.dumps({"error": "Invalid Attribute"})

    # Function to implement set
    def setValue(self, path, value, attribute="value", timeout=5):
        return self.setValues({path: value}, attribute, timeout)

    def setValues(self, updates: Dict[str, Any], attribute="value", timeout=5):
        if attribute in self.AttrDict:
            field, _ = self.AttrDict[attribute]
            entry_updates = []
            for path, value in updates.items():
                if field is kuksa_client.grpc.Field.VALUE:
                    entry = kuksa_client.grpc.DataEntry(
                        path=path, value=kuksa_client.grpc.Datapoint(value=value))
                elif field is kuksa_client.grpc.Field.ACTUATOR_TARGET:
                    entry = kuksa_client.grpc.DataEntry(
                        path=path, actuator_target=kuksa_client.grpc.Datapoint(
                            value=value),
                    )
                elif field is kuksa_client.grpc.Field.METADATA:
                    try:
                        metadata_dict = json.loads(value)
                    except json.JSONDecodeError:
                        return json.dumps({"error": "Metadata value needs to be a valid JSON object"})
                    entry = kuksa_client.grpc.DataEntry(
                        path=path, metadata=kuksa_client.grpc.Metadata.from_dict(
                            metadata_dict),
                    )
                entry_updates.append(kuksa_client.grpc.EntryUpdate(
                    entry=entry, fields=(field,)))
            requestArgs = {'updates': entry_updates}
            return self._sendReceiveMsg(("set", requestArgs), timeout)
        return json.dumps({"error": "Invalid Attribute"})

    # Function for authorization
    def authorize(self, tokenfile=None, timeout=5):
        if tokenfile is None:
            tokenfile = self.tokenfile
        tokenfile = pathlib.Path(tokenfile)
        requestArgs = {
            'token': tokenfile.expanduser().read_text(encoding='utf-8').rstrip('\n')
        }

        return self._sendReceiveMsg(("authorize", requestArgs), timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated.
    def subscribe(self, path, callback, attribute="value", timeout=5):
        return self.subscribeMultiple((path,), callback, attribute, timeout)

    def subscribeMultiple(self, paths, callback, attribute="value", timeout=5):
        if attribute in self.AttrDict:
            field, view = self.AttrDict[attribute]
            entries = [kuksa_client.grpc.SubscribeEntry(
                path=path, view=view, fields=(field,)) for path in paths]
            requestArgs = {
                'entries': entries,
                'callback': callback_wrapper(callback),
            }
            return self._sendReceiveMsg(("subscribe", requestArgs), timeout)

        return json.dumps({"error": "Invalid Attribute"})

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, sub_id, timeout=5):
        try:
            sub_id = uuid.UUID(sub_id)
        except ValueError as exc:
            return json.dumps({"error": str(exc)})
        requestArgs = {'subscription_id': sub_id}
        return self._sendReceiveMsg(("unsubscribe", requestArgs), timeout)

    def connect(self, timeout=5):
        requestArgs = {}
        return self._sendReceiveMsg(("connect", requestArgs), timeout)

    def disconnect(self, timeout=5):
        requestArgs = {}
        return self._sendReceiveMsg(("disconnect", requestArgs), timeout)

    def _sendReceiveMsg(self, req, timeout):
        (call, requestArgs) = req
        recvQueue = queue.Queue(maxsize=1)
        self.sendMsgQueue.put((call, requestArgs, recvQueue))
        try:
            resp, error = recvQueue.get(timeout=timeout)
            if error:
                respJson = json.dumps(error, indent=4)
            elif resp:
                respJson = json.dumps(resp, indent=4)
            else:
                respJson = "OK"
        except queue.Empty:
            respJson = json.dumps({"error": "Timeout"})
        return respJson

    # Async function to handle the gRPC calls
    async def _grpcHandler(self, vss_client):
        self.grpcConnected = True
        self.run = True
        subscriber_manager = kuksa_client.grpc.aio.SubscriberManager(
            vss_client)
        while self.run:
            try:
                (call, requestArgs, responseQueue) = self.sendMsgQueue.get_nowait()
            except queue.Empty:
                await asyncio.sleep(0.01)
                continue
            try:
                if call == "get":
                    resp = await vss_client.get(**requestArgs)
                    if resp is not None:
                        resp = [entry.to_dict() for entry in resp]
                        resp = resp[0] if len(resp) == 1 else resp
                elif call == "set":
                    resp = await vss_client.set(**requestArgs)
                elif call == "authorize":
                    resp = await vss_client.authorize(str(requestArgs["token"]))
                elif call == "subscribe":
                    callback = requestArgs.pop('callback')
                    subscriber_response_stream = vss_client.subscribe(
                        **requestArgs)
                    resp = await subscriber_manager.add_subscriber(subscriber_response_stream, callback)
                    resp = {"subscriptionId": str(resp)}
                elif call == "unsubscribe":
                    resp = await subscriber_manager.remove_subscriber(**requestArgs)
                elif call == "connect":
                    resp = await vss_client.connect()
                elif call == "disconnect":
                    resp = await vss_client.disconnect()
                else:
                    raise Exception("Not Implemented.")

                responseQueue.put((resp, None))
            except kuksa_client.grpc.VSSClientError as exc:
                responseQueue.put((None, exc.to_dict()))
            except ValueError as exc:
                responseQueue.put(
                    (None, {"error": "ValueError in casting the value."}))

        self.grpcConnected = False

    # Main loop for handling gRPC communication
    async def mainLoop(self):
        if self.insecure:

            async with kuksa_client.grpc.aio.VSSClient(self.serverIP, self.serverPort, token=self.token) as vss_client:
                print("gRPC channel connected.")
                await self._grpcHandler(vss_client)
        else:
            async with kuksa_client.grpc.aio.VSSClient(
                self.serverIP,
                self.serverPort,
                root_certificates=self.cacertificate,
                private_key=self.keyfile,
                certificate_chain=self.certificate,
                token=self.token
            ) as vss_client:
                print("Secure gRPC channel connected.")
                await self._grpcHandler(vss_client)
