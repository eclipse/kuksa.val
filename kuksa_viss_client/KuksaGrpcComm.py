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

from http import client
import queue, os
import grpc, asyncio, json
from google.protobuf.json_format import MessageToJson

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

        self.AttrDict = {
            "value": kuksa_pb2.RequestType.CURRENT_VALUE,
            "targetValue": kuksa_pb2.RequestType.TARGET_VALUE,
            "metadata": kuksa_pb2.RequestType.METADATA
        }

        self.grpcConnectionAuthToken = None

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

    # Function to implement get
    def getValue(self, path, attribute="value", timeout = 5):
        recvQueue = queue.Queue(maxsize=1)

        # Create Get Request
        req = kuksa_pb2.GetRequest()
        if attribute in self.AttrDict.keys():
            pass
        else:
            attribute = "value"
        req.type = self.AttrDict[attribute]
        req.path.append(path)
        self.sendMsgQueue.put(("get", req, recvQueue))
        resp = recvQueue.get(timeout=timeout)
        respJson = MessageToJson(resp)
        return respJson        

    # Function to implement set
    def setValue(self, path, value, attribute="value", timeout = 5):
        recvQueue = queue.Queue(maxsize=1)

        # Create Set Request
        req = kuksa_pb2.SetRequest()
        if attribute in self.AttrDict.keys():
            pass
        else:
            attribute = "value"
        req.type = self.AttrDict[attribute]
        val = kuksa_pb2.Value()
        val.path = path
        val.valueString = value
        req.values.append(val)

        self.sendMsgQueue.put(("set", req, recvQueue))
        resp = recvQueue.get(timeout=timeout)
        respJson = MessageToJson(resp)
        return respJson

    # Function for authorization
    def authorize(self, token=None, timeout=2):
        recvQueue = queue.Queue(maxsize=1)
        if token == None:
            token = self.tokenfile
        token = os.path.expanduser(token)
        if os.path.isfile(token):
            with open(token, "r") as f:
                token = f.readline()

        # Create AuthRequest
        req = kuksa_pb2.AuthRequest()
        req.token = token
        self.sendMsgQueue.put(("authorize", req, recvQueue))
        resp = recvQueue.get(timeout=timeout)
        respJson = MessageToJson(resp)
        self.grpcConnectionAuthToken = resp.connectionId
        return respJson


    # Async function to handle the gRPC calls
    async def _grpcHandler(self, clientStub):
        
        # gRPC Socket is connected
        self.grpcConnected = True
        self.run = True
        while self.run:
            try:
                (call, requestObj, responseQueue) = self.sendMsgQueue.get_nowait()
            except queue.Empty:
                await asyncio.sleep(0.01)
                continue
            except Exception as e:
                print("gRPCHandler Exception: " + str(e))
                continue

            try:
                md = (("connectionid", self.grpcConnectionAuthToken),)
                if call == "get":
                    respObj = await clientStub.get(requestObj, metadata=md)
                elif call == "set":
                    respObj = await clientStub.set(requestObj, metadata=md)
                elif call == "authorize":
                    respObj = await clientStub.authorize(requestObj, metadata=md)        
                else:
                    raise Exception("Not Implemented!!")
                responseQueue.put(respObj)
            except Exception as e:
                print("gRPCHandler Exception: " + str(e))
                
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
                # Use the gRPC Stubs
                clientStub = kuksa_pb2_grpc.kuksa_grpc_ifStub(channel)
                await self._grpcHandler(clientStub)

