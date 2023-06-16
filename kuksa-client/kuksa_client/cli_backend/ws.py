#! /usr/bin/env python

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
import os.path
import pathlib
import queue
import ssl
import time
import uuid

import websockets

from kuksa_client import cli_backend

class Backend(cli_backend.Backend):
    def __init__(self, config):
        super().__init__(config)
        self.wsConnected = False

        self.subscriptionCallbacks = {}
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueues = {}
        self.run = False

    async def _receiver_handler(self, webSocket):
        while self.run:
            message = await webSocket.recv()
            resJson = json.loads(message)
            if "requestId" in resJson:
                try:
                    self.recvMsgQueues[resJson["requestId"]].put(message)
                finally:
                    del self.recvMsgQueues[resJson["requestId"]]
            else:
                if "subscriptionId" in resJson and resJson["subscriptionId"] in self.subscriptionCallbacks:
                    try:
                        self.subscriptionCallbacks[resJson["subscriptionId"]](
                            message)
                    except Exception as e:  # pylint: disable=broad-except
                        print(e)

    async def _sender_handler(self, webSocket):
        while self.run:
            try:
                req = self.sendMsgQueue.get_nowait()
                await webSocket.send(req)
            except queue.Empty:
                await asyncio.sleep(0.01)
            except Exception as e:  # pylint: disable=broad-except
                print(e)
                return

    async def _msgHandler(self, webSocket):
        self.wsConnected = True
        self.run = True
        recv = asyncio.Task(self._receiver_handler(webSocket))
        send = asyncio.Task(self._sender_handler(webSocket))

        await asyncio.wait([recv, send], return_when=asyncio.FIRST_COMPLETED)
        recv.cancel()
        send.cancel()

        await webSocket.close()

    # Internal function to send and receive messages on websocket
    def _sendReceiveMsg(self, req, timeout):
        req["requestId"] = str(uuid.uuid4())
        jsonDump = json.dumps(req)
        sent = False

        # Register the receive queue before sending
        recvQueue = queue.Queue(maxsize=1)
        self.recvMsgQueues[req["requestId"]] = recvQueue
        while not sent:
            try:
                self.sendMsgQueue.put_nowait(jsonDump)
                sent = True
            except queue.Full:
                time.sleep(0.01)

        # Wait on the receive queue
        try:
            res = recvQueue.get(timeout=timeout)
            resJson = json.loads(res)
            if "requestId" in res and str(req["requestId"]) == str(resJson["requestId"]):
                return json.dumps(resJson, indent=2)
        except queue.Empty:
            req["error"] = "timeout"
            return json.dumps(req, indent=2)

    # Function to stop the communication
    def stop(self):
        self.wsConnected = False
        self.run = False
        print("Server disconnected.")

    def disconnect(self):
        self.stop()

    # Function to authorize against the kuksa.val server
    def authorize(self, token_or_tokenfile=None, timeout=2):
        if token_or_tokenfile is None:
            token_or_tokenfile = self.token_or_tokenfile
        if os.path.isfile(token_or_tokenfile):
            token_or_tokenfile = pathlib.Path(token_or_tokenfile)
            token = token_or_tokenfile.expanduser().read_text(encoding='utf-8')
        else:
            token = token_or_tokenfile
        req = {}
        req["action"] = "authorize"
        req["tokens"] = token
        return self._sendReceiveMsg(req, timeout)

    # Update VSS Tree Entry
    def updateVSSTree(self, jsonStr, timeout=5):
        req = {}
        req["action"] = "updateVSSTree"
        if os.path.isfile(jsonStr):
            with open(jsonStr, "r", encoding="utf-8") as f:
                req["metadata"] = json.load(f)
        else:
            req["metadata"] = json.loads(jsonStr)
        return self._sendReceiveMsg(req, timeout)

   # Update Meta Data of a given path
    def updateMetaData(self, path, jsonStr, timeout=5):
        req = {}
        req["action"] = "updateMetaData"
        req["path"] = path
        req["metadata"] = json.loads(jsonStr)
        return self._sendReceiveMsg(req, timeout)

    # Get Meta Data of a given path
    def getMetaData(self, path, timeout=5):
        """Get MetaData of the parameter"""
        req = {}
        req["action"] = "getMetaData"
        req["path"] = path
        return self._sendReceiveMsg(req, timeout)

    # Set value to a given path
    def setValue(self, path, value, attribute="value", timeout=5):
        if 'nan' == value:
            return json.dumps({"error": path + " has an invalid value " + str(value)}, indent=2)
        req = {}
        req["action"] = "set"
        req["path"] = path
        req["attribute"] = attribute
        try:
            jsonValue = json.loads(value)
            if isinstance(jsonValue, list):
                req[attribute] = []
                for v in jsonValue:
                    req[attribute].append(str(v))
            else:
                req[attribute] = str(value)
        except json.decoder.JSONDecodeError:
            req[attribute] = str(value)

        return self._sendReceiveMsg(req, timeout)

    # Get value to a given path
    def getValue(self, path, attribute="value", timeout=5):
        req = {}
        req["action"] = "get"
        req["path"] = path
        req["attribute"] = attribute
        return self._sendReceiveMsg(req, timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, attribute="value", timeout=5):
        req = {}
        req["action"] = "subscribe"
        req["path"] = path
        req["attribute"] = attribute
        res = self._sendReceiveMsg(req, timeout)
        resJson = json.loads(res)
        if "subscriptionId" in resJson:
            self.subscriptionCallbacks[resJson["subscriptionId"]] = callback
        return res

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, subId, timeout=5):
        req = {}
        req["action"] = "unsubscribe"
        req["subscriptionId"] = subId

        res = {}
        # Check if the subscription subId exists
        if subId in self.subscriptionCallbacks:
            # No matter what happens, remove the callback
            del self.subscriptionCallbacks[subId]
            res = self._sendReceiveMsg(req, timeout)
        else:
            errMsg = {}
            errMsg["number"] = "404"
            errMsg["message"] = "Could not unsubscribe"
            errMsg["reason"] = "Subscription ID does not exist"
            res["error"] = errMsg
            res = json.dumps(res)

        return res

    # Function to check connection
    def checkConnection(self):
        return self.wsConnected

    async def connect(self):
        if not self.insecure:
            context = ssl.create_default_context()
            context.load_cert_chain(
                certfile=self.certificate, keyfile=self.keyfile)
            context.load_verify_locations(cafile=self.cacertificate)
            # We want host name to match
            # For example certificates we use subjectAltName to make it match for Server, localahost and 127.0.0.1
            # If using your own certificates make sure that name is included or use tls_server_name work-around
            context.check_hostname = True
            try:
                print("connect to wss://"+self.serverIP+":"+str(self.serverPort))
                args = {'uri':"wss://"+self.serverIP+":"+str(self.serverPort),'ssl':context}
                # If your certificates does not contain the name of the server 
                # (for example during testing, in production it should match)
                # then you can specify that checks should be against a different name
                if self.tls_server_name:
                    args['server_hostname'] = self.tls_server_name

                async with websockets.connect(**args) as ws:
                    print("Websocket connected securely.")
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
        else:
            try:
                print("connect to ws://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    print("Websocket connected.")
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))

    # Main loop for handling websocket communication
    async def mainLoop(self):
        await self.connect()
