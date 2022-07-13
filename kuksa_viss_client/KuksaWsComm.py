#! /usr/bin/env python

########################################################################
# Copyright (c) 2022 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################

import json, queue, time, uuid, os, ssl
import asyncio, websockets

class KuksaWsComm:

    # Constructor
    def __init__(self, config):

        scriptDir= os.path.dirname(os.path.realpath(__file__))
        self.serverIP = config.get('ip', "127.0.0.1")
        self.serverPort = config.get('port', 8090)
        try:
            self.insecure = config.getboolean('insecure', False)
        except AttributeError:
            self.insecure = config.get('insecure', False)
        self.cacertificate = config.get('cacertificate', os.path.join(scriptDir, "../kuksa_certificates/CA.pem"))
        self.certificate = config.get('certificate', os.path.join(scriptDir, "../kuksa_certificates/Client.pem"))
        self.keyfile = config.get('key', os.path.join(scriptDir, "../kuksa_certificates/Client.key"))
        self.wsConnected = False

        self.subscriptionCallbacks = {}
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueues = {}

    async def _receiver_handler(self, webSocket):
        while self.run:
            message = await webSocket.recv()
            resJson = json.loads(message) 
            if "requestId" in resJson:
                try:
                    self.recvMsgQueues[resJson["requestId"]].put(message)
                    del(self.recvMsgQueues[resJson["requestId"]])
                except:
                    del(self.recvMsgQueues[resJson["requestId"]])
            else:
                if "subscriptionId" in resJson and resJson["subscriptionId"] in self.subscriptionCallbacks:
                    try:
                        self.subscriptionCallbacks[resJson["subscriptionId"]](message)
                    except Exception as e:
                        print(e)


    async def _sender_handler(self, webSocket):
        while self.run:
            try:
                req = self.sendMsgQueue.get_nowait()
                await webSocket.send(req)
            except queue.Empty:
                await asyncio.sleep(0.01)
            except Exception as e:
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
            resJson =  json.loads(res) 
            if "requestId" in res and str(req["requestId"]) == str(resJson["requestId"]):
                return res
        except queue.Empty:
            req["error"] =  "timeout"
            return json.dumps(req, indent=2)

    # Function to stop the communication
    def stop(self):
        self.wsConnected = False
        self.run = False
        print("Server disconnected.")

    # Function to authorize against the kuksa.val server
    def authorize(self, token="", timeout=2):
        token = os.path.expanduser(token)
        if os.path.isfile(token):
            with open(token, "r") as f:
                token = f.readline()

        req = {}
        req["action"]= "authorize"
        req["tokens"] = token
        print(f"Sending token {token}")
        return self._sendReceiveMsg(req, timeout)

    # Update VSS Tree Entry 
    def updateVSSTree(self, jsonStr, timeout = 5):
        req = {}
        req["action"]= "updateVSSTree"
        if os.path.isfile(jsonStr):
            with open(jsonStr, "r") as f:
                req["metadata"] = json.load(f)
        else:
            req["metadata"] = json.loads(jsonStr) 
        return self._sendReceiveMsg(req, timeout)

   # Update Meta Data of a given path
    def updateMetaData(self, path, jsonStr, timeout = 5):
        req = {}
        req["action"]= "updateMetaData"
        req["path"] = path
        req["metadata"] = json.loads(jsonStr) 
        return self._sendReceiveMsg(req, timeout)

    # Get Meta Data of a given path
    def getMetaData(self, path, timeout = 1):
        """Get MetaData of the parameter"""
        req = {}
        req["action"]= "getMetaData"
        req["path"] = path 
        return self._sendReceiveMsg(req, timeout)

    # Set value to a given path
    def setValue(self, path, value, attribute="value", timeout = 1):
        if 'nan' == value:
            print(path + " has an invalid value " + str(value))
            return
        req = {}
        req["action"]= "set"
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
    def getValue(self, path, attribute="value", timeout = 5):
        req = {}
        req["action"] = "get"
        req["path"] = path
        req["attribute"] = attribute
        return self._sendReceiveMsg(req, timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, attribute = "value", timeout = 5):
        req = {}
        req["action"]= "subscribe"
        req["path"] = path
        req["attribute"] = attribute
        res = self._sendReceiveMsg(req, timeout)
        resJson =  json.loads(res) 
        if "subscriptionId" in resJson:
            self.subscriptionCallbacks[resJson["subscriptionId"]] = callback 
        return res

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, id, timeout = 5):
        req = {}
        req["action"]= "unsubscribe"
        req["subscriptionId"] = id

        res = {}
        # Check if the subscription id exists
        if id in self.subscriptionCallbacks.keys():
            # No matter what happens, remove the callback
            del(self.subscriptionCallbacks[id]) 
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
        if self.wsConnected:
            return True
        else:
            return False

    # Main loop for handling websocket communication
    async def mainLoop(self):
        if not self.insecure:
            context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            context.load_cert_chain(certfile=self.certificate, keyfile=self.keyfile)
            context.load_verify_locations(cafile=self.cacertificate)
            try:
                print("connect to wss://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("wss://"+self.serverIP+":"+str(self.serverPort), ssl=context) as ws:
                    print("Websocket connected securely.")
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass
        else:
            try:
                print("connect to ws://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    print("Websocket connected.")
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass