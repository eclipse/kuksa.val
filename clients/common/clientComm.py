#! /usr/bin/python3

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################

import sys, threading, queue, ssl
import asyncio, websockets, pathlib

class VSSClientComm(threading.Thread):

    # Constructor
    def __init__(self, sendMsgQueue, recvMsgQueue, config):
        super(VSSClientComm, self).__init__()
        self.sendMsgQueue = sendMsgQueue
        self.recvMsgQueue = recvMsgQueue
        self.serverIP = config.get('ip', "127.0.0.1")
        self.serverPort = config.get('port', 8090)
        self.insecure = config.getboolean('insecure', False)
        self.interval=config.getint('interval',1)
        self.cacertificate = config.get('cacertificate', "CA.pem")
        self.certificate = config.get('certificate', "Client.pem")
        self.keyfile = config.get('key', "Client.key")
        self.runComm = True
        self.wsConnected = False

    def stopComm(self):
        self.runComm = False
        self.wsConnected = False

    async def msgHandler(self, webSocket):
        while self.runComm:
            try:
                req = self.sendMsgQueue.get(timeout=1)
                await webSocket.send(req)
                resp = await webSocket.recv()
                self.recvMsgQueue.put(resp)
            except queue.Empty:
                pass

    async def mainLoop(self):
        if not self.insecure:
            context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            context.load_cert_chain(certfile=self.certificate, keyfile=self.keyfile)
            context.load_verify_locations(cafile=self.cacertificate)
            try:
                print("connect to wss://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("wss://"+self.serverIP+":"+str(self.serverPort), ssl=context) as ws:
                    self.wsConnected = True
                    await self.msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass
        else:
            try:
                print("connect to ws://"+self.serverIP+":"+str(self.serverPort))
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    self.wsConnected = True
                    await self.msgHandler(ws)
            except OSError as e:
                print("Disconnected!! " + str(e))
                pass

    # Thread function: Start the asyncio loop
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.mainLoop())
