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

from http import client
from pydoc import cli
import queue, os
import grpc, asyncio, json
from google.protobuf.json_format import MessageToJson
import jsonpath_ng
import uuid, time, threading

from kuksa.val import kuksa_pb2
from kuksa.val import kuksa_pb2_grpc

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
        self.subscriptionIdMap = {}
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueues = {}
        self.run = False

        self.AttrDict = {
            "value": kuksa_pb2.RequestType.CURRENT_VALUE,
            "targetValue": kuksa_pb2.RequestType.TARGET_VALUE,
            "metadata": kuksa_pb2.RequestType.METADATA
        }

        self.grpcConnectionAuthToken = None

    # Get datatype of a path
    def getDataType(self, path):

        mdJson = self.getMetaData(path)
        md = json.loads(mdJson)
        print
        if "error" not in md:
            dt = [dt.value for dt in jsonpath_ng.parse("$..datatype").find(md)]
            if len(dt) == 1:
                datatype = dt[0]
                return datatype
        else:
            return None


    # Function to check connection status
    def checkConnection(self):
        if self.grpcConnected:
            return True
        else:
            return False

    # Function to stop the communication
    def stop(self):
        self.run = False
        print("gRPC channel disconnected.")

    # Function to implement fetching of metadata
    def getMetaData(self, path, timeout = 5):
        recvQueue = queue.Queue(maxsize=1)
        # Create Get Request
        req = kuksa_pb2.GetRequest()
        req.type = kuksa_pb2.RequestType.METADATA
        req.path.append(path)
        self.sendMsgQueue.put(("get", req, (recvQueue,)))
        finalrespJson = {}
        try:
            resp = recvQueue.get(timeout=timeout)
            respJson = MessageToJson(resp)
            resp = json.loads(respJson)
            finalrespJson["metadata"] = json.loads(resp["values"][0]["valueString"])
            finalrespJson = json.dumps(finalrespJson, indent=4)
        except queue.Empty:
            finalrespJson = json.dumps({"error": "Timeout"})
    
        return finalrespJson 

    # Function to implement get
    def getValue(self, path, attribute="value", timeout = 5):
        recvQueue = queue.Queue(maxsize=1)

        # Create Get Request
        req = kuksa_pb2.GetRequest()
        if attribute in self.AttrDict.keys():
            req.type = self.AttrDict[attribute]
            req.path.append(path)
            self.sendMsgQueue.put(("get", req, (recvQueue,)))
            try:
                resp = recvQueue.get(timeout=timeout)
                respJson = MessageToJson(resp)
            except queue.Empty:
                respJson = json.dumps({"error": "Timeout"})
        else:
            respJson = json.dumps({"error": "Invalid Attribute"})
        return respJson 

    # Function to implement set
    def setValue(self, path, value, attribute="value", timeout = 5):
        recvQueue = queue.Queue(maxsize=1)

        # Create Set Request
        req = kuksa_pb2.SetRequest()
        if attribute in self.AttrDict.keys():
            dt = self.getDataType(path)
            req.type = self.AttrDict[attribute]
            val = kuksa_pb2.Value()
            val.path = path

            if dt=="int8" or dt=="int16" or dt=="int32":
                val.valueInt32 = int(value)
            elif dt=="uint8" or dt=="uint16" or dt=="uint32": 
                val.valueUint32 = int(value)
            elif dt=="uint64":
                val.valueUint64 = int(value)
            elif dt=="int64":
                val.valueInt64 = int(value)
            elif dt=="float":
                val.valueFloat= float(value)
            elif dt=="double":
                val.valueDouble = float(value)
            elif dt=="boolean":
                if value in ["False", "false", "F", "f"]:
                    value = 0
                val.valueBool = bool(value)
            else:
                val.valueString = str(value)

            req.values.append(val)

            self.sendMsgQueue.put(("set", req, (recvQueue,)))
            try:
                resp = recvQueue.get(timeout=timeout)
                respJson = MessageToJson(resp)
            except queue.Empty:
                respJson = json.dumps({"error": "Timeout"})
        else:
            respJson = json.dumps({"error": "Invalid Attribute"})

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
        self.sendMsgQueue.put(("authorize", req, (recvQueue,)))
        try: 
            resp = recvQueue.get(timeout=timeout)
            self.grpcConnectionAuthToken = resp.connectionId
            respJson = MessageToJson(resp)
        except queue.Empty:
            respJson = json.dumps({"error": "Timeout"})
        return respJson

    # Unsubscribe value changes of to a given path.
    # The subscription id from the response of the corresponding subscription request will be required
    def unsubscribe(self, id, timeout = 5):

        recvQueue = queue.Queue(maxsize=1)
        self.sendMsgQueue.put(("unsubscribe", id, (recvQueue,)))
        try: 
            resp = recvQueue.get(timeout=timeout)
            respJson = json.dumps(resp)
        except queue.Empty:
            respJson = json.dumps({"error": "Timeout"})
        return respJson
        

    # Subscribe value changes of to a given path.
    # The given callback function will be called then, if the given path is updated:
    #   updateMessage = await webSocket.recv()
    #   callback(updateMessage)
    def subscribe(self, path, callback, attribute = "value", timeout = 5):
        recvQueue = queue.Queue(maxsize=1)

        # Create Get Request
        req = kuksa_pb2.SubscribeRequest()
        if attribute in self.AttrDict.keys():
            req.type = self.AttrDict[attribute]
            req.path = path
            req.start = bool(True)
            self.sendMsgQueue.put(("subscribe", req, (recvQueue, callback)))
            try:
                (resp, subsId) = recvQueue.get(timeout=timeout)
                respJson = MessageToJson(resp)
                resp = json.loads(respJson)
                resp["subscriptionId"] = subsId
                respJson = json.dumps(resp)
            except queue.Empty:
                respJson = json.dumps({"error": "Timeout"})
        else:
            respJson = json.dumps({"error": "Invalid Attribute"})
        return respJson 

    # Async function to handle the gRPC calls
    async def _grpcHandler(self, clientStub):
        
        # gRPC Socket is connected
        self.grpcConnected = True
        self.run = True
        while self.run:
            try:
                (call, requestObj, options) = self.sendMsgQueue.get_nowait()
                responseQueue = options[0]
            except queue.Empty:
                await asyncio.sleep(0.01)
                continue
            except Exception as e:
                print("gRPCHandler Exception: " + str(e))
                continue

            try:
                if self.grpcConnectionAuthToken != None:
                    md = (("connectionid", self.grpcConnectionAuthToken),)
                else:
                    md = ()
                if call == "get":
                    respObj = await clientStub.get(requestObj, metadata=md)
                elif call == "set":
                    respObj = await clientStub.set(requestObj, metadata=md)
                elif call == "authorize":
                    respObj = await clientStub.authorize(requestObj, metadata=md)    
                elif call == "unsubscribe":
                    if requestObj in self.subscriptionCallbacks:
                        del (self.subscriptionCallbacks[requestObj])
                    if requestObj in self.subscriptionIdMap:
                        cv = self.subscriptionIdMap[requestObj]
                        async with cv:
                            cv.notify()
                        del (self.subscriptionIdMap[requestObj])
                        respObj = { "status": {
                                "statusCode": 200,
                                "statusDescription": "Unsubscribe request successfully processed."
                                }
                            }   
                    else:
                        respObj = { "status": {
                                "statusCode": 400,
                                "statusDescription": "Unsubscribe request failed."
                                }
                            }
                elif call == "subscribe":
                    callback = options[1]
                    respObj = None
                    cv = asyncio.Condition()

                    # Function to be executed by the subscribe thread.
                    async def subscribeThread(callback, cv, requestObj):

                        async def iterator(cv, requestObj):
                            yield requestObj
                            requestObj.start = False
                            async with cv:
                                await cv.wait()
                            yield requestObj
                            return

                        call = clientStub.subscribe(iterator(cv, requestObj), metadata=md)
                        async for resp in call:        
                            if resp.HasField("values"):
                                callback(MessageToJson(resp))  
                            else:
                                respObj = resp
                                if resp.status.statusCode == 200:
                                    subsId = str(uuid.uuid4())
                                    self.subscriptionCallbacks[subsId] = callback 
                                    self.subscriptionIdMap[subsId] = cv
                                    responseQueue.put((respObj, subsId))
                                else:
                                    responseQueue.put((respObj, None))
                    
                    loop = asyncio.get_event_loop()
                    t = threading.Thread(target=loop.create_task, args=(subscribeThread(callback, cv, requestObj),))
                    t.start()
                    await asyncio.sleep(1)
                else:
                    raise Exception("Not Implemented.")

                if respObj != None:
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
                print("gRPC channel connected.")
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
                print("Secure gRPC channel created.")
                # Use the gRPC Stubs
                clientStub = kuksa_pb2_grpc.kuksa_grpc_ifStub(channel)
                await self._grpcHandler(clientStub)

