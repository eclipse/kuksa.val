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

import queue, os
import grpc, asyncio

import kuksa_pb2
import kuksa_pb2_grpc

class KuksaGrpcComm:

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
        self.tokenfile = config.get('token', os.path.join(scriptDir, "../kuksa_certificates/jwt/all-read-write.json.token"))
        self.grpcConnected = False

        self.subscriptionCallbacks = {}
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueues = {}
        self.run = False

    # Function to check connection status
    def checkConnection(self):
        if self.grpcConnected:
            return True
        else:
            return False

    # Function to stop the communication
    def stop(self):
        self.run = False
        print("gRPC channel disconnected!!")

    # Receive handler
    async def _receiver_handler(self, clientStub):

        while self.run:
            await asyncio.sleep(0.1)

    # Send handler
    async def _sender_handler(self, clientStub):

        while self.run:
            await asyncio.sleep(0.1)

    # Async function to handle the gRPC calls
    async def _grpcHandler(self, clientStub):
        # gRPC Socket is connected
        self.grpcConnected = True
        self.run = True
        recv = asyncio.Task(self._receiver_handler(clientStub))
        send = asyncio.Task(self._sender_handler(clientStub))

        await asyncio.wait([recv, send], return_when=asyncio.FIRST_COMPLETED)
        recv.cancel()
        send.cancel()
        self.grpcConnected = False

    # Main loop for handling gRPC communication
    async def mainLoop(self):
        serverAddr = self.serverIP + ":" + str(self.serverPort)
        if self.insecure:
            # Insecure mode
            async with grpc.aio.insecure_channel(serverAddr) as channel:
                print("gRPC channel connected!!")
                # Use the gRPC stubs
                clientStub = kuksa_pb2_grpc.kuksa_grpc_ifStub(channel)
                await self._grpcHandler(clientStub)
        else:
            # Secure mode
            f = open(self.cacertificate, "rb")
            caCert = f.read()
            f.close()
            f = open(self.certificate, "rb")
            clientCert = f.read()
            f.close()
            f = open(self.keyfile, "rb")
            clientKey = f.read()
            f.close()

            creds = grpc.ssl_channel_credentials(root_certificates=caCert, private_key=clientKey, certificate_chain=clientCert)
            async with grpc.aio.secure_channel(serverAddr, creds) as channel:
                print("Secure gRPC channel created!!")
                # USe the gRPC Stubs
                clientStub = kuksa_pb2_grpc.kuksa_grpc_ifStub(channel)
                await self._grpcHandler(clientStub)

