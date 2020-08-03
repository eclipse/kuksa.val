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

import argparse, json, sys
from cmd2 import Cmd, with_argparser, with_category
import queue, time
from clientComm import VSSClientComm

DEFAULT_SERVER_ADDR = "localhost"
DEFAULT_SERVER_PORT = 8090

ap_getServerAddr = argparse.ArgumentParser()
ap_connect = argparse.ArgumentParser()
ap_connect.add_argument('-i', "--insecure", default=False, action="store_true", help='Connect in insecure mode')
ap_disconnect = argparse.ArgumentParser()
ap_authorize = argparse.ArgumentParser()
ap_authorize.add_argument('Token', help='JWT for authorizing the client.')
ap_setServerAddr = argparse.ArgumentParser()
ap_setServerAddr.add_argument('IP', help='VSS Server IP Address', default=DEFAULT_SERVER_ADDR)
ap_setServerAddr.add_argument('Port', type=int, help='VSS Server Websocket Port', default=DEFAULT_SERVER_PORT)
ap_setValue = argparse.ArgumentParser()
ap_setValue.add_argument("Parameter", help="Parameter to be set")
ap_setValue.add_argument("Value", help="Value to be set")
ap_getValue = argparse.ArgumentParser()
ap_getValue.add_argument("Parameter", help="Parameter whose metadata is to be read")
ap_getMetadata = argparse.ArgumentParser()
ap_getMetadata.add_argument("Parameter", help="Parameter whose metadata is to be read")


class VSSTestClient(Cmd):

    COMM_SETUP_COMMANDS = "Communication Set-up Commands"
    VSS_COMMANDS = "VSS Interaction Commands"

    # Constructor
    def __init__(self):
        super(VSSTestClient, self).__init__(persistent_history_file=".vssclient_history", persistent_history_length=100)
        self.prompt = "VSS Client> "
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueue = queue.Queue()
        self.serverIP = DEFAULT_SERVER_ADDR
        self.serverPort = DEFAULT_SERVER_PORT


    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_authorize)
    def do_authorize(self, args):
        """Authorize the client to interact with the server"""
        token = args.Token
        req = {}
        req["requestId"] = 1238
        req["action"]= "authorize"
        req["tokens"] = token
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        resp = self.recvMsgQueue.get()
        print(resp)

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setValue)
    def do_setValue(self, args):
        """Set the value of a parameter"""
        req = {}
        req["requestId"] = 1235
        req["action"]= "set"
        req["path"] = args.Parameter
        req["value"] = args.Value
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        resp = self.recvMsgQueue.get()
        print(resp)

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getValue)
    def do_getValue(self, args):
        """Get the value of a parameter"""
        req = {}
        req["requestId"] = 1234
        req["action"]= "get"
        req["path"] = args.Parameter
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        resp = self.recvMsgQueue.get()
        print(resp)


    def do_quit(self, args):
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stopComm()
                time.sleep(1)
        super(VSSTestClient, self).do_quit(args)
        sys.exit(0)

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getMetadata)
    def do_getMetadata(self, args):
        """Get Metadata of the parameter"""
        req = {}
        req["requestId"] = 1236
        req["action"]= "getMetadata"
        req["path"] = args.Parameter
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        resp = self.recvMsgQueue.get()
        print(resp)


    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_disconnect)
    def do_disconnect(self, args):
        """Disconnect from the VSS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stopComm()
                self.commThread = None
            print("Websocket disconnected!!")

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_connect)
    def do_connect(self, args):
        """Connect to the VSS Server"""
        self.commThread = VSSClientComm(self.serverIP, self.serverPort, self.sendMsgQueue, self.recvMsgQueue, args.insecure)
        self.commThread.start()

        pollIndex = 10
        while(pollIndex > 0):
            if self.commThread.wsConnected == True:
                pollIndex = 0
            else:
                time.sleep(0.1)
            pollIndex -= 1

        if self.commThread.wsConnected:
            print("Websocket connected!!")
        else:
            print("Websocket could not be connected!!")
            self.commThread.stopComm()
            self.commThread = None

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_setServerAddr)
    def do_setServerAddress(self, args):
        """Sets the IP Address for the VSS Server"""
        try:
            self.serverIP = args.IP
            self.serverPort = args.Port
            print("Setting Server Address to " + args.IP + ":" + str(args.Port))
        except ValueError:
            print("Please give a valid server Address")

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_getServerAddr)
    def do_getServerAddress(self, args):
        """Gets the IP Address for the VSS Server"""
        if hasattr(self, "serverIP") and hasattr(self, "serverPort"):
            print(self.serverIP + ":" + str(self.serverPort))
        else:
            print("Server IP not set!!")

# Main Function
if __name__=="__main__":
    import sys
    clientApp = VSSTestClient()
    sys.exit(clientApp.cmdloop())
