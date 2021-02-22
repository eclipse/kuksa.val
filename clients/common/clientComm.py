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

import os, sys, threading, queue, ssl, json, time
import uuid
import asyncio, websockets, pathlib

class VSSClientComm(threading.Thread):

    # Constructor
    def __init__(self, config):
        super(VSSClientComm, self).__init__()
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueue = queue.Queue()
        self.callbacks = {}
        self.tasks = []
        scriptDir= os.path.dirname(os.path.realpath(__file__))
        self.serverIP = config.get('ip', "127.0.0.1")
        self.serverPort = config.get('port', 8090)
        try:
            self.insecure = config.getboolean('insecure', False)
        except AttributeError:
            self.insecure = config.get('insecure', False)
        self.cacertificate = config.get('cacertificate', os.path.join(scriptDir, "CA.pem"))
        self.certificate = config.get('certificate', os.path.join(scriptDir, "Client.pem"))
        self.keyfile = config.get('key', os.path.join(scriptDir, "Client.key"))
        self.wsConnected = False

    def stopComm(self):
        self.wsConnected = False
        for t in self.tasks:
            t.cancel()

    def sendReceiveMsg_(self, req, timeout): 
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
            

    def authorize(self, token, timeout = 2):
        if os.path.isfile(token):
            with open(token, "r") as f:
                token = f.readline()

        req = {}
        req["action"]= "authorize"
        req["tokens"] = token
        return self.sendReceiveMsg_(req, timeout)

    def updateVSSTree(self, jsonStr, timeout = 5):
        req = {}
        req["action"]= "updateVSSTree"
        if os.path.isfile(jsonStr):
            with open(jsonStr, "r") as f:
                req["metadata"] = json.load(f)
        else:
            req["metadata"] = json.loads(jsonStr) 
        return self.sendReceiveMsg_(req, timeout)

    def updateMetaData(self, path, jsonStr, timeout = 5):
        req = {}
        req["action"]= "updateMetaData"
        req["path"] = path
        req["metadata"] = json.loads(jsonStr) 
        return self.sendReceiveMsg_(req, timeout)

    def getMetaData(self, path, timeout = 1):
        """Get MetaData of the parameter"""
        req = {}
        req["action"]= "getMetaData"
        req["path"] = path 
        return self.sendReceiveMsg_(req, timeout)

    def setValue(self, path, value, timeout = 1):
        if 'nan' == value:
            print(path + " has an invalid value " + str(value))
            return
        req = {}
        req["action"]= "set"
        req["path"] = path
        req["value"] = value
        return self.sendReceiveMsg_(req, timeout)


    def getValue(self, path, timeout = 5):
        req = {}
        req["action"]= "get"
        req["path"] = path
        return self.sendReceiveMsg_(req, timeout)

    def subscribe(self, path, callback, timeout = 5):
        req = {}
        req["action"]= "subscribe"
        req["path"] = path
        res = self.sendReceiveMsg_(req, timeout)
        resJson =  json.loads(res) 
        if "subscriptionId" in resJson:
            self.callbacks[resJson["subscriptionId"]] = callback; 
        return res;


    async def receiver_handler_(self, webSocket):
        while True:
            message = await webSocket.recv()
            resJson =  json.loads(message) 
            if "requestId" in resJson:
                self.recvMsgQueue.put(message)
            else:
                if "subscriptionId" in resJson and resJson["subscriptionId"] in self.callbacks:
                    self.callbacks[resJson["subscriptionId"]](message)

    async def sender_handler_(self, webSocket):
        while True:
            try:
                req = self.sendMsgQueue.get(timeout=0.1)
                await webSocket.send(req)
            except queue.Empty:
                await asyncio.sleep(0.1)
                pass
    
    async def msgHandler(self, webSocket):
        self.wsConnected = True
        self.tasks = [
            asyncio.ensure_future(self.receiver_handler_(webSocket)),
            asyncio.ensure_future(self.sender_handler_(webSocket))
        ]

        try:
            await asyncio.gather(
                *self.tasks, 
                return_exceptions=False
            )

        except asyncio.exceptions.CancelledError as e:
            print("all tasks canceled")


        finally:
            await webSocket.close()

    async def mainLoop(self):
        if not self.insecure:
            context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            context.load_cert_chain(certfile=self.certificate, keyfile=self.keyfile)
            context.load_verify_locations(cafile=self.cacertificate)
            try:
                print("connect to wss://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("wss://"+self.serverIP+":"+str(self.serverPort), ssl=context) as ws:
                    await self.msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass
        else:
            try:
                print("connect to ws://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    await self.msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass

    # Thread function: Start the asyncio loop
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.mainLoop())
