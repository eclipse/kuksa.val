#! /usr/bin/env python
########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
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

import argparse, json, sys
from re import sub
from typing import Dict, List
import queue, time, os, threading
from pygments import highlight, lexers, formatters
from cmd2 import Cmd, with_argparser, with_category, Cmd2ArgumentParser, CompletionItem
from cmd2.utils import CompletionError, basic_complete
import functools, subprocess
DEFAULT_SERVER_ADDR = "127.0.0.1"
DEFAULT_SERVER_PORT = 8090
DEFAULT_SERVER_PROTOCOL = "ws"

from grpc.tools import command
# Generate the proto Stubs
scriptDir= os.path.dirname(os.path.realpath(__file__))
command.build_package_protos(os.path.join(scriptDir, "kuksa.proto"))

sys.path.append(os.path.join(scriptDir, ".."))

from kuksa_client import KuksaClientThread
from kuksa_client._metadata import *
import kuksa_certificates

class TestClient(Cmd):
    def get_childtree(self, pathText):
        childVssTree = self.vssTree

        paths = [pathText]
        if "/" in pathText:
            paths = pathText.split("/")
        elif "." in pathText:
            paths = pathText.split(".")

        for path in paths[:-1]:
            if path in childVssTree:
                childVssTree = childVssTree[path]
            elif 'children' in childVssTree and path in childVssTree['children']:
                childVssTree = childVssTree['children'][path]
            else:
                # This else-branch is reached when one of the path components is invalid
                # In that case stop parsing further and return an empty tree
                # Autocompletion can't help here.
                childVssTree={}
                break

        if 'children' in childVssTree:
                childVssTree = childVssTree['children']
        return childVssTree

    def path_completer(self, text, line, begidx, endidx):
        if not self.checkConnection():
            return
        if len(self.pathCompletionItems) == 0:
            tree = json.loads(self.getMetaData("*"))

            if 'metadata' in tree:
                self.vssTree = tree['metadata']

        self.pathCompletionItems = []
        childTree = self.get_childtree(text)
        prefix = ""
        seperator="/"

        if "/" in text:
            prefix = text[:text.rfind("/")]+"/"
        elif  "." in text:
            prefix = text[:text.rfind(".")]+"."
            seperator="."

        for key in childTree:
            child = childTree[key]
            if isinstance(child, dict):
                description = ""
                nodetype = "unknown"

                if 'description' in child:
                    description = child['description']

                if 'type' in child:
                    nodetype=child['type'].capitalize()

                self.pathCompletionItems.append(CompletionItem(prefix + key, nodetype+": "+ description))

                if 'children' in child:
                    self.pathCompletionItems.append(CompletionItem(prefix + key+seperator, "Children of branch "+prefix+key))

        return basic_complete(text, line, begidx, endidx, self.pathCompletionItems)

    def subscribeCallback(self, path, attr, resp):
        self.subscribeFileDesc[(path,attr)].write(resp + "\n")
        self.subscribeFileDesc[(path,attr)].flush()

    def subscriptionIdCompleter(self, text, line, begidx, endidx):
        self.pathCompletionItems = []
        for id in self.subscribeIdToPath.keys():
            self.pathCompletionItems.append(CompletionItem(id))
        return basic_complete(text, line, begidx, endidx, self.pathCompletionItems)

    COMM_SETUP_COMMANDS = "Communication Set-up Commands"
    VSS_COMMANDS = "Kuksa Interaction Commands"
    INFO_COMMANDS = "Info Commands"

    ap_getServerAddr = argparse.ArgumentParser()
    ap_connect = argparse.ArgumentParser()
    ap_connect.add_argument('-i', "--insecure", default=False, action="store_true", help='Connect in insecure mode')
    ap_disconnect = argparse.ArgumentParser()
    ap_authorize = argparse.ArgumentParser()
    tokenfile_completer_method = functools.partial(Cmd.path_complete,
        path_filter=lambda path: (os.path.isdir(path) or path.endswith(".token")))
    ap_authorize.add_argument('Token', help='JWT(or the file storing the token) for authorizing the client.', completer_method=tokenfile_completer_method)
    ap_setServerAddr = argparse.ArgumentParser()
    ap_setServerAddr.add_argument('IP', help='VISS/gRPC Server IP Address', default=DEFAULT_SERVER_ADDR)
    ap_setServerAddr.add_argument('Port', type=int, help='VISS/gRPC Server Port', default=DEFAULT_SERVER_PORT)
    ap_setServerAddr.add_argument('-p', "--protocol", help='VISS/gRPC Server Communication Protocol (ws or grpc)', default=DEFAULT_SERVER_PROTOCOL)

    ap_setValue = argparse.ArgumentParser()
    ap_setValue.add_argument("Path", help="Path to be set", completer_method=path_completer)
    ap_setValue.add_argument("Value", help="Value to be set")
    ap_setValue.add_argument("-a", "--attribute", help="Attribute to be set", default="value")

    ap_getValue = argparse.ArgumentParser()
    ap_getValue.add_argument("Path", help="Path to be read", completer_method=path_completer)
    ap_getValue.add_argument("-a", "--attribute", help="Attribute to be get", default="value")

    ap_setTargetValue = argparse.ArgumentParser()
    ap_setTargetValue.add_argument("Path", help="Path whose target value to be set", completer_method=path_completer)
    ap_setTargetValue.add_argument("Value", help="Value to be set")

    ap_getTargetValue = argparse.ArgumentParser()
    ap_getTargetValue.add_argument("Path", help="Path whose target value to be read", completer_method=path_completer)

    ap_subscribe = argparse.ArgumentParser()
    ap_subscribe.add_argument("Path", help="Path to be subscribed", completer_method=path_completer)
    ap_subscribe.add_argument("-a", "--attribute", help="Attribute to be subscribed", default="value")

    ap_unsubscribe = argparse.ArgumentParser()
    ap_unsubscribe.add_argument("SubscribeId", help="Corresponding subscription Id", completer_method=subscriptionIdCompleter)

    ap_getMetaData = argparse.ArgumentParser()
    ap_getMetaData.add_argument("Path", help="Path whose metadata is to be read", completer_method=path_completer)
    ap_updateMetaData = argparse.ArgumentParser()
    ap_updateMetaData.add_argument("Path", help="Path whose MetaData is to update", completer_method=path_completer)
    ap_updateMetaData.add_argument("Json", help="MetaData to update. Note, only attributes can be update, if update children or the whole vss tree, use `updateVSSTree` instead.")

    ap_updateVSSTree = argparse.ArgumentParser()
    jsonfile_completer_method = functools.partial(Cmd.path_complete,
        path_filter=lambda path: (os.path.isdir(path) or path.endswith(".json")))
    ap_updateVSSTree.add_argument("Json", help="Json tree to update VSS", completer_method=jsonfile_completer_method)

    # Constructor
    def __init__(self):
        super(TestClient, self).__init__(persistent_history_file=".vssclient_history", persistent_history_length=100)

        self.prompt = "Test Client> "
        self.max_completion_items = 20
        self.serverIP = DEFAULT_SERVER_ADDR
        self.serverPort = DEFAULT_SERVER_PORT
        self.serverProtocol = DEFAULT_SERVER_PROTOCOL
        # Supported set of protocols
        self.supportedProtocols = ["ws", "grpc"]

        self.vssTree = {}
        self.pathCompletionItems = []
        self.subscribeFileDesc = {}
        self.subscribeIdToPath = {}

        print("Welcome to Kuksa Client version " + str(__version__))
        print()
        with open(os.path.join(scriptDir, 'logo'), 'r') as f:
            print(f.read())
        print("Default tokens directory: " + self.getDefaultTokenDir())

        print()
        self.connect()

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_authorize)
    def do_authorize(self, args):
        """Authorize the client to interact with the server"""
        if self.checkConnection():
            resp = self.commThread.authorize(args.Token)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setValue)
    def do_setValue(self, args):
        """Set the value of a path"""
        if self.checkConnection():
            resp = self.commThread.setValue(args.Path, args.Value, args.attribute)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setTargetValue)
    def do_setTargetValue(self, args):
        """Set the value of a path"""
        if self.checkConnection():
            resp = self.commThread.setValue(args.Path, args.Value, "targetValue")
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getValue)
    def do_getValue(self, args):
        """Get the value of a path"""
        if self.checkConnection():
            resp = self.commThread.getValue(args.Path, args.attribute)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getTargetValue)
    def do_getTargetValue(self, args):
        """Get the value of a path"""
        if self.checkConnection():
            resp = self.commThread.getValue(args.Path, "targetValue")
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_subscribe)
    def do_subscribe(self, args):
        """Subscribe the value of a path"""
        if self.checkConnection():

            resp = self.commThread.subscribe(args.Path, lambda msg: self.subscribeCallback(args.Path, args.attribute, msg), args.attribute)
            resJson =  json.loads(resp)
            if "subscriptionId" in resJson:
                fileName = os.getcwd() + "/log_"+args.Path.replace("/", ".")+"_"+args.attribute+"_"+str(time.time())
                self.subscribeFileDesc[(args.Path, args.attribute)] = open(fileName, "w")
                self.subscribeIdToPath[resJson["subscriptionId"]] = (args.Path, args.attribute)
                print("Subscription log available at " + fileName)
                print("Execute tail -f " + fileName + " on another Terminal instance")
                from shutil import which
                if which("xterm") != None:
                    subprocess.Popen(["xterm", "-e", "/bin/bash", "-l", "-c", "tail -f " + fileName])
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_unsubscribe)
    def do_unsubscribe(self, args):
        """Unsubscribe an existing subscription"""
        if self.checkConnection():
            resp = self.commThread.unsubscribe(args.SubscribeId)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
            if args.SubscribeId in self.subscribeIdToPath.keys():
                (path,attr) = self.subscribeIdToPath[args.SubscribeId]
                if path in self.subscribeFileDesc:
                    self.subscribeFileDesc[(path,attr)].close()
                    del(self.subscribeFileDesc[(path,attr)])
                del(self.subscribeIdToPath[args.SubscribeId])
            self.pathCompletionItems = []

    def do_quit(self, args):
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                time.sleep(1)
        super(TestClient, self).do_quit(args)
        sys.exit(0)

    def getMetaData(self, path):
        """Get MetaData of the path"""
        if self.checkConnection():
            return self.commThread.getMetaData(path)
        else:
            return "{}"

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_updateVSSTree)
    def do_updateVSSTree(self, args):
        """Update VSS Tree Entry"""
        if self.checkConnection():
            resp =  self.commThread.updateVSSTree(args.Json)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_updateMetaData)
    def do_updateMetaData(self, args):
        """Update MetaData of a given path"""
        if self.checkConnection():
            resp =  self.commThread.updateMetaData(args.Path, args.Json)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getMetaData)
    def do_getMetaData(self, args):
        """Get MetaData of the path"""
        resp = self.getMetaData(args.Path)
        print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []


    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_disconnect)
    def do_disconnect(self, args):
        """Disconnect from the VISS/gRPC Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None

    def checkConnection(self):
        if None == self.commThread or not self.commThread.checkConnection():
            self.connect()
        return self.commThread.checkConnection()

    def connect(self, insecure=False):
        """Connect to the VISS/gRPC Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
        config = {'ip':self.serverIP,
        'port': self.serverPort,
        'insecure' : insecure,
        'protocol': self.serverProtocol
        }
        self.commThread = KuksaClientThread(config)
        self.commThread.start()

        waitForConnection = threading.Condition()
        waitForConnection.acquire()
        waitForConnection.wait_for(lambda: self.commThread.checkConnection()==True, timeout=1)
        waitForConnection.release()

        if self.commThread.checkConnection():
            pass
        else:
            print("Error: Websocket could not be connected or the gRPC channel could not be created.")
            self.commThread.stop()
            self.commThread = None

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_connect)
    def do_connect(self, args):
        self.connect(args.insecure)

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_setServerAddr)
    def do_setServerAddress(self, args):
        """Sets the IP Address for the VISS/gRPC Server"""
        try:
            self.serverIP = args.IP
            self.serverPort = args.Port
            if args.protocol not in self.supportedProtocols:
                raise ValueError
            self.serverProtocol = args.protocol
            print("Setting Server Address to " + args.IP + ":" + str(args.Port) + " with protocol " + args.protocol)
        except ValueError:
            print("Error: Please give a valid server Address/Protocol. Only ws and grpc are supported.")

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_getServerAddr)
    def do_getServerAddress(self, args):
        """Gets the IP Address for the VISS/gRPC Server"""
        if hasattr(self, "serverIP") and hasattr(self, "serverPort"):
            print(self.serverIP + ":" + str(self.serverPort))
        else:
            print("Server IP not set!!")

    def getDefaultTokenDir(self):
        try:
            return os.path.join(kuksa_certificates.__certificate_dir__, "jwt")
        except Exception:
            guessTokenDir = os.path.join(scriptDir, "../kuksa_certificates/jwt")
            if os.path.isdir(guessTokenDir):
                return guessTokenDir
            return "Unknown"

    @with_category(INFO_COMMANDS)
    def do_info(self, args):
        """Show summary info of the client"""
        print("kuksa-client version " + __version__)
        print("Uri: " + __uri__)
        print("Author: " + __author__)
        print("Copyright: " + __copyright__)
        print("Default tokens directory: " + self.getDefaultTokenDir())

    @with_category(INFO_COMMANDS)
    def do_version(self, args):
        """Show version of the client"""
        print(__version__)

    @with_category(INFO_COMMANDS)
    def do_printTokenDir(self, args):
        """Show default token directory"""
        print(self.getDefaultTokenDir())

# Main Function
def main():
    clientApp = TestClient()
    clientApp.cmdloop()

if __name__=="__main__":
    sys.exit(main())

