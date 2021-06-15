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

import os, sys, threading, queue, ssl, json
import uuid
import asyncio, websockets, pathlib
scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, ".."))
from kuksa_viss_client._metadata import *

class KuksaClientThread(threading.Thread):

    # Constructor
    def __init__(self, config):
        super(KuksaClientThread, self).__init__()
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueue = queue.Queue()
        self.subscriptionCallbacks = {}
        self.handlerTasks = []
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
        self.tokenfile = config.get('token', os.path.join(scriptDir, "../kuksa_certificates/jwt/all-read-write.json.token"))
        self.wsConnected = False

    def stop(self):
        self.wsConnected = False
        self.run = False


    def _sendReceiveMsg(self, req, timeout): 
        req["requestId"] = str(uuid.uuid4())
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        while True:
            try:
                res = self.recvMsgQueue.get(timeout = timeout)
                resJson =  json.loads(res) 
                if "requestId" in res and str(req["requestId"]) == str(resJson["requestId"]):
                    return res
            except queue.Empty:
                req["error"] =  "timeout"
                return json.dumps(req, indent=2) 
            

    # Do authorization by passing a jwt token or a token file
    def authorize(self, token=None, timeout = 2):
        if token == None:
            token = self.tokenfile
        token = os.path.expanduser(token)
        print("token is " + token)
        if os.path.isfile(token):
            with open(token, "r") as f:
                token = f.readline()

        req = {}
        req["action"]= "authorize"
        req["tokens"] = token
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
    def setValue(self, path, value, timeout = 1):
        if 'nan' == value:
            print(path + " has an invalid value " + str(value))
            return
        req = {}
        req["action"]= "set"
        req["path"] = path
        req["value"] = value
        return self._sendReceiveMsg(req, timeout)


    # Get value to a given path
    def getValue(self, path, timeout = 5):
        req = {}
        req["action"]= "get"
        req["path"] = path
        return self._sendReceiveMsg(req, timeout)

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, timeout = 5):
        req = {}
        req["action"]= "subscribe"
        req["path"] = path
        res = self._sendReceiveMsg(req, timeout)
        resJson =  json.loads(res) 
        if "subscriptionId" in resJson:
            self.subscriptionCallbacks[resJson["subscriptionId"]] = callback; 
        return res


    async def _receiver_handler(self, webSocket):
        while self.run:
            message = await webSocket.recv()
            resJson = json.loads(message) 
            if "requestId" in resJson:
                self.recvMsgQueue.put(message)
            else:
                if "subscriptionId" in resJson and resJson["subscriptionId"] in self.subscriptionCallbacks:
                    self.subscriptionCallbacks[resJson["subscriptionId"]](message)

    async def _sender_handler(self, webSocket):
        while self.run:
            try:
                req = self.sendMsgQueue.get(timeout=0.1)
                await webSocket.send(req)
            except queue.Empty:
                await asyncio.sleep(0.1)
                pass
    
    async def _msgHandler(self, webSocket):
        self.wsConnected = True
        self.run = True
        recv = asyncio.Task(self._receiver_handler(webSocket))
        send = asyncio.Task(self._sender_handler(webSocket))

        await asyncio.wait([recv, send], return_when=asyncio.FIRST_COMPLETED)
        recv.cancel()
        send.cancel()
        
        await webSocket.close()

    async def mainLoop(self):
        if not self.insecure:
            context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            context.load_cert_chain(certfile=self.certificate, keyfile=self.keyfile)
            context.load_verify_locations(cafile=self.cacertificate)
            try:
                print("connect to wss://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("wss://"+self.serverIP+":"+str(self.serverPort), ssl=context) as ws:
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass
        else:
            try:
                print("connect to ws://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    await self._msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass

    # Thread function: Start the asyncio loop
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.mainLoop())
