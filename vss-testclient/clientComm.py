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

import threading, queue, ssl
import asyncio, websockets, pathlib

class VSSClientComm(threading.Thread):

    # Constructor
    def __init__(self, ip, port, sendMsgQueue, recvMsgQueue, insecure):
        super(VSSClientComm, self).__init__()
        self.serverIP = ip
        self.serverPort = port
        self.sendMsgQueue = sendMsgQueue
        self.recvMsgQueue = recvMsgQueue
        self.runComm = True
        self.wsConnected = False
        self.insecure = insecure

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
            context.load_cert_chain(certfile="Client.pem", keyfile="Client.key")
            context.load_verify_locations(cafile="CA.pem")
            try:
                async with websockets.connect("wss://"+self.serverIP+":"+str(self.serverPort), ssl=context) as ws:
                    self.wsConnected = True
                    await self.msgHandler(ws)
            except OSError as e:
                print("Disconnected!!" + str(e))
                pass
        else:
            try:
                async with websockets.connect("ws://"+self.serverIP+":"+str(self.serverPort)) as ws:
                    self.wsConnected = True
                    await self.msgHandler(ws)
            except OSError:
                pass

    # Thread function: Start the asyncio loop
    def run(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self.mainLoop())
