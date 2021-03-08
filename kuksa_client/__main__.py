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
import argparse, json, sys
from typing import Dict, List
import queue, time, os
from pygments import highlight, lexers, formatters
from cmd2 import Cmd, with_argparser, with_category, Cmd2ArgumentParser, CompletionItem
from cmd2.utils import CompletionError, basic_complete
import functools
DEFAULT_SERVER_ADDR = "127.0.0.1"
DEFAULT_SERVER_PORT = 8090

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, ".."))
from kuksa_client import KuksaClientThread

class TestClient(Cmd):
    def get_childtree(self, pathText):
        childVssTree = self.vssTree
        if "." in pathText:
            paths = pathText.split(".")
            for path in paths:
                if path in childVssTree:
                    childVssTree = childVssTree[path]
                elif 'children' in childVssTree and path in childVssTree['children']:
                    childVssTree = childVssTree['children'][path]
            if 'children' in childVssTree:
                childVssTree = childVssTree['children']
        return childVssTree

    def path_completer(self, text, line, begidx, endidx):
        if len(self.pathCompletionItems) == 0:
            tree = json.loads(self.getMetaData("*"))
                
            if 'metadata' in tree:
                self.vssTree = tree['metadata']

        self.pathCompletionItems = []
        childTree = self.get_childtree(text)
        prefix = ""
        if "." in text:
            prefix = text[:text.rfind(".")]+"."
        for key in childTree:
            description = ""
            if 'description' in childTree[key]:
                description = "("+childTree[key]['description']+")"
            self.pathCompletionItems.append(CompletionItem(prefix + key, description))
            if 'children' in childTree[key]:
                self.pathCompletionItems.append(CompletionItem(prefix + key + ".", "(children...)"))

        return basic_complete(text, line, begidx, endidx, self.pathCompletionItems)

    COMM_SETUP_COMMANDS = "Communication Set-up Commands"
    VISS_COMMANDS = "Kuksa Interaction Commands"

    ap_getServerAddr = argparse.ArgumentParser()
    ap_connect = argparse.ArgumentParser()
    ap_connect.add_argument('-i', "--insecure", default=False, action="store_true", help='Connect in insecure mode')
    ap_disconnect = argparse.ArgumentParser()
    ap_authorize = argparse.ArgumentParser()
    tokenfile_completer_method = functools.partial(Cmd.path_complete,
        path_filter=lambda path: (os.path.isdir(path) or path.endswith(".token")))
    ap_authorize.add_argument('Token', help='JWT(or the file storing the token) for authorizing the client.', completer_method=tokenfile_completer_method)
    ap_setServerAddr = argparse.ArgumentParser()
    ap_setServerAddr.add_argument('IP', help='VISS Server IP Address', default=DEFAULT_SERVER_ADDR)
    ap_setServerAddr.add_argument('Port', type=int, help='VISS Server Websocket Port', default=DEFAULT_SERVER_PORT)

    ap_setValue = argparse.ArgumentParser()
    ap_setValue.add_argument("Path", help="Path to be set", completer_method=path_completer)
    ap_setValue.add_argument("Value", help="Value to be set")

    ap_getValue = argparse.ArgumentParser()
    ap_getValue.add_argument("Path", help="Path whose metadata is to be read", completer_method=path_completer)
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
        self.vssTree = {}
        self.pathCompletionItems = []
        self.connect()


    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_authorize)
    def do_authorize(self, args):
        """Authorize the client to interact with the server"""
        if self.checkConnection():
            resp = self.commThread.authorize(args.Token)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VISS_COMMANDS)
    @with_argparser(ap_setValue)
    def do_setValue(self, args):
        """Set the value of a path"""
        if self.checkConnection():
            resp = self.commThread.setValue(args.Path, args.Value)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VISS_COMMANDS)
    @with_argparser(ap_getValue)
    def do_getValue(self, args):
        """Get the value of a path"""
        if self.checkConnection():
            resp = self.commThread.getValue(args.Path)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
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

    @with_category(VISS_COMMANDS)
    @with_argparser(ap_updateVSSTree)
    def do_updateVSSTree(self, args):
        """Update VSS Tree Entry"""
        if self.checkConnection():
            resp =  self.commThread.updateVSSTree(args.Json)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VISS_COMMANDS)
    @with_argparser(ap_updateMetaData)
    def do_updateMetaData(self, args):
        """Update MetaData of a given path"""
        if self.checkConnection():
            resp =  self.commThread.updateMetaData(args.Path, args.Json)
            print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))

    @with_category(VISS_COMMANDS)
    @with_argparser(ap_getMetaData)
    def do_getMetaData(self, args):
        """Get MetaData of the path"""
        resp = self.getMetaData(args.Path)
        print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []


    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_disconnect)
    def do_disconnect(self, args):
        """Disconnect from the VISS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
            print("Websocket disconnected!!")

    def checkConnection(self):
        if None == self.commThread or not self.commThread.wsConnected: 
            self.connect()
        return self.commThread.wsConnected


    def connect(self, insecure=False):
        """Connect to the VISS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
        config = {'ip':self.serverIP,
        'port': self.serverPort,
        'insecure' : insecure
        }
        self.commThread = KuksaClientThread(config)
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
            self.commThread.stop()
            self.commThread = None

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_connect)
    def do_connect(self, args):
        self.connect(args.insecure)

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_setServerAddr)
    def do_setServerAddress(self, args):
        """Sets the IP Address for the VISS Server"""
        try:
            self.serverIP = args.IP
            self.serverPort = args.Port
            print("Setting Server Address to " + args.IP + ":" + str(args.Port))
        except ValueError:
            print("Please give a valid server Address")

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_getServerAddr)
    def do_getServerAddress(self, args):
        """Gets the IP Address for the VISS Server"""
        if hasattr(self, "serverIP") and hasattr(self, "serverPort"):
            print(self.serverIP + ":" + str(self.serverPort))
        else:
            print("Server IP not set!!")

# Main Function
def main():
    clientApp = TestClient()
    clientApp.cmdloop()

if __name__=="__main__":
    sys.exit(main())

