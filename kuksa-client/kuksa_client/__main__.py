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

import argparse
import functools
import json
import logging.config
import logging
import os
import pathlib
import sys
import threading
import time

from pygments import highlight
from pygments import lexers
from pygments import formatters
from cmd2 import Cmd
from cmd2 import CompletionItem
from cmd2 import with_argparser
from cmd2 import with_category
from cmd2.utils import basic_complete
from urllib.parse import urlparse

import kuksa_certificates
from kuksa_client import KuksaClientThread
from kuksa_client import _metadata

DEFAULT_SERVER_ADDR = "grpc://127.0.0.1:55555"

scriptDir = os.path.dirname(os.path.realpath(__file__))


logger = logging.getLogger(__name__)


def assignment_statement(arg):
    path, value = arg.split('=', maxsplit=1)
    return (path, value)


def display_completions(completions, delimiter):
    # Index of what prefix to remove from displayed items
    # I.e. "Vehicle." should be removed if the common prefix is "Vehicle.Ve".
    prefix_idx = os.path.commonprefix(completions).rfind(delimiter) + 1
    matches = []
    for path in completions:
        path = path[prefix_idx:]
        # Only display completions up to (and including) the next delimiter.
        next_dot = path.find(delimiter)
        if next_dot != -1:
            path = path[:next_dot+1]
        matches.append(path)
    return matches


def metadata_tree_to_dict(tree):
    def add_children(flattened_tree, path, value):
        if "children" in value:
            for child_path, value in value["children"].items():
                add_children(flattened_tree, f"{path}.{child_path}", value)
        else:
            flattened_tree[path] = value
    flattened_tree = {}

    for key, value in tree.items():
        add_children(flattened_tree, key, value)

    return flattened_tree


# pylint: disable=too-many-instance-attributes
# pylint: disable=too-many-public-methods
class TestClient(Cmd):
    def refresh_metadata(self):
        if self.server.startswith("grpc"):
            entries = json.loads(self.getMetaData("**"))
            if 'error' in entries:
                raise Exception("Wrong databroker version, please use a newer version")
            # Convert to dict with paths as key
            self.metadata = {entry["path"]: entry for entry in entries}
        else:
            entries = json.loads(self.getMetaData("*"))
            if 'metadata' in entries:
                # Convert to dict with paths as key
                self.metadata = metadata_tree_to_dict(entries['metadata'])

    def path_completer(self, text, line, begidx, endidx):
        if not self.checkConnection():
            return None

        if len(self.pathCompletionItems) == 0:
            self.refresh_metadata()

        # Normalize the delimiter used
        delimiter = "."
        if "/" in text:
            delimiter = "/"
            text = text.replace(delimiter, ".")

        # Generate the list of all possible completions
        self.pathCompletionItems = []
        for path in self.metadata.keys():
            # Compare case insensitive
            if path.lower().startswith(text.lower()):
                if delimiter != ".":
                    path = path.replace(".", delimiter)
                self.pathCompletionItems.append(path)

        # Generate the list of completions to display
        self.display_matches = display_completions(self.pathCompletionItems, delimiter)

        # Return all completions
        return self.pathCompletionItems

    def subscribeCallback(self, logPath, resp):
        if logPath is None:
            self.async_alert(highlight(json.dumps(json.loads(resp), indent=2),
                             lexers.JsonLexer(), formatters.TerminalFormatter()))
        else:
            with logPath.open('a', encoding='utf-8') as logFile:
                logFile.write(resp + "\n")

    def subscriptionIdCompleter(self, text, line, begidx, endidx):
        self.pathCompletionItems = []
        for sub_id in self.subscribeIds:
            self.pathCompletionItems.append(CompletionItem(sub_id))
        return basic_complete(text, line, begidx, endidx, self.pathCompletionItems)

    COMM_SETUP_COMMANDS = "Communication Set-up Commands"
    VSS_COMMANDS = "Kuksa Interaction Commands"
    INFO_COMMANDS = "Info Commands"

    ap_connect = argparse.ArgumentParser()
    ap_connect.add_argument(
        'server', help=f"VSS server to connect to. Format: protocol://host[:port]. \
        Supported protocols: [grpc, grpcs, ws, wss]. Example: {DEFAULT_SERVER_ADDR}",)

    ap_disconnect = argparse.ArgumentParser()
    ap_authorize = argparse.ArgumentParser()
    tokenfile_completer_method = functools.partial(
        Cmd.path_complete,
        path_filter=lambda path: (os.path.isdir(path) or path.endswith(".token"))
    )
    ap_authorize.add_argument(
        'token_or_tokenfile',
        help='JWT(or the file storing the token) for authorizing the client.',
        completer_method=tokenfile_completer_method,)

    ap_setValue = argparse.ArgumentParser()
    ap_setValue.add_argument(
        "Path", help="Path to be set", completer_method=path_completer)
    ap_setValue.add_argument("Value", nargs='+', help="Value to be set")
    ap_setValue.add_argument(
        "-a", "--attribute", help="Attribute to be set", default="value")

    ap_setValues = argparse.ArgumentParser()
    ap_setValues.add_argument(
        "Path=Value",
        help="Path and new value this path is to be set with",
        nargs='+',
        type=assignment_statement,
    )
    ap_setValues.add_argument(
        "-a", "--attribute", help="Attribute to be set", default="value")

    ap_getValue = argparse.ArgumentParser()
    ap_getValue.add_argument(
        "Path", help="Path to be read", completer_method=path_completer)
    ap_getValue.add_argument(
        "-a", "--attribute", help="Attribute to be get", default="value")

    ap_getValues = argparse.ArgumentParser()
    ap_getValues.add_argument(
        "Path", help="Path whose value is to be read", nargs='+', completer_method=path_completer)
    ap_getValues.add_argument(
        "-a", "--attribute", help="Attribute to be get", default="value")

    ap_setTargetValue = argparse.ArgumentParser()
    ap_setTargetValue.add_argument(
        "Path", help="Path whose target value to be set", completer_method=path_completer)
    ap_setTargetValue.add_argument("Value", help="Value to be set")

    ap_setTargetValues = argparse.ArgumentParser()
    ap_setTargetValues.add_argument(
        "Path=Value",
        help="Path and new target value this path is to be set with",
        nargs='+',
        type=assignment_statement,
    )

    ap_getTargetValue = argparse.ArgumentParser()
    ap_getTargetValue.add_argument(
        "Path", help="Path whose target value is to be read", completer_method=path_completer)

    ap_getTargetValues = argparse.ArgumentParser()
    ap_getTargetValues.add_argument(
        "Path", help="Path whose target value is to be read", nargs='+', completer_method=path_completer)

    ap_subscribe = argparse.ArgumentParser()
    ap_subscribe.add_argument(
        "Path", help="Path to subscribe to", completer_method=path_completer)
    ap_subscribe.add_argument(
        "-a", "--attribute", help="Attribute to subscribe to", default="value")

    ap_subscribe.add_argument(
        "-f", "--output-to-file", help="Redirect the subscription output to file", action="store_true")

    ap_subscribeMultiple = argparse.ArgumentParser()
    ap_subscribeMultiple.add_argument(
        "Path", help="Path to subscribe to", nargs='+', completer_method=path_completer)
    ap_subscribeMultiple.add_argument(
        "-a", "--attribute", help="Attribute to subscribe to", default="value")
    ap_subscribeMultiple.add_argument(
        "-f", "--output-to-file", help="Redirect the subscription output to file", action="store_true")

    ap_unsubscribe = argparse.ArgumentParser()
    ap_unsubscribe.add_argument(
        "SubscribeId", help="Corresponding subscription Id", completer_method=subscriptionIdCompleter,
    )

    ap_getMetaData = argparse.ArgumentParser()
    ap_getMetaData.add_argument(
        "Path", help="Path whose metadata is to be read", completer_method=path_completer)
    ap_updateMetaData = argparse.ArgumentParser()
    ap_updateMetaData.add_argument(
        "Path", help="Path whose MetaData is to update", completer_method=path_completer)
    ap_updateMetaData.add_argument(
        "Json",
        help="MetaData to update. Note, only attributes can be update, if update children or the whole vss tree, use"
        " `updateVSSTree` instead.",
    )

    ap_updateVSSTree = argparse.ArgumentParser()
    jsonfile_completer_method = functools.partial(
        Cmd.path_complete,
        path_filter=lambda path: (os.path.isdir(path) or path.endswith(".json"))
    )
    ap_updateVSSTree.add_argument(
        "Json", help="Json tree to update VSS", completer_method=jsonfile_completer_method)

    # Constructor, request names after protocol to avoid errors
    def __init__(self, server=None, token_or_tokenfile=None,
                 certificate=None, keyfile=None,
                 cacertificate=None, tls_server_name=None):
        super().__init__(
            persistent_history_file=".vssclient_history", persistent_history_length=100, allow_cli_args=False,
        )

        self.prompt = "Test Client> "
        self.max_completion_items = 20
        self.server = server or DEFAULT_SERVER_ADDR

        self.metadata = {}
        self.pathCompletionItems = []
        self.subscribeIds = set()
        self.commThread = None
        self.token_or_tokenfile = token_or_tokenfile
        self.certificate = certificate
        self.keyfile = keyfile
        self.cacertificate = cacertificate
        self.tls_server_name = tls_server_name

        print("Welcome to Kuksa Client version " + str(_metadata.__version__))
        print()
        with (pathlib.Path(scriptDir) / 'logo').open('r', encoding='utf-8') as f:
            print(f.read())
        print("Default tokens directory: " + self.getDefaultTokenDir())

        print()
        self.connect()

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_authorize)
    def do_authorize(self, args):
        """Authorize the client to interact with the server"""
        if args.token_or_tokenfile is not None:
            self.token_or_tokenfile = args.token_or_tokenfile
        if self.checkConnection():
            resp = self.commThread.authorize(self.token_or_tokenfile)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setValue)
    def do_setValue(self, args):
        """Set the value of a path"""
        if self.checkConnection():
            # If there is a blank before a single/double quote on the kuksa-client cli then
            # the argparser shell will remove it, there is nothing we can do to it
            # This gives off behavior for examples like:
            # setValue Vehicle.OBD.DTCList [ "dtc1, dtc2", ddd]
            # which will be treated as input of 3 elements
            # The recommended approach is to have quotes (of a different type) around the whole value
            # if your strings includes quotes, commas or other items
            # setValue Vehicle.OBD.DTCList '[ "dtc1, dtc2", ddd]'
            # or
            # setValue Vehicle.OBD.DTCList "[ 'dtc1, dtc2', ddd]"
            # If you really need to include a quote in the values use backslash and use the quote type
            # you want as inner value:
            # setValue Vehicle.OBD.DTCList "[ 'dtc1, \'dtc2', ddd]"
            # Will result in two elements in the array; "dtc1, 'dtc2" and "ddd"
            value = str(' '.join(args.Value))
            resp = self.commThread.setValue(
                args.Path, value, args.attribute)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setValues)
    def do_setValues(self, args):
        """Set the value of given paths"""
        if self.checkConnection():
            resp = self.commThread.setValues(
                dict(getattr(args, 'Path=Value')), args.attribute)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setTargetValue)
    def do_setTargetValue(self, args):
        """Set the target value of a path"""
        if self.checkConnection():
            resp = self.commThread.setValue(
                args.Path, args.Value, "targetValue")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_setTargetValues)
    def do_setTargetValues(self, args):
        """Set the target value of given paths"""
        if self.checkConnection():
            resp = self.commThread.setValues(
                dict(getattr(args, 'Path=Value')), "targetValue")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getValue)
    def do_getValue(self, args):
        """Get the value of a path"""
        if self.checkConnection():
            resp = self.commThread.getValue(args.Path, args.attribute)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getValues)
    def do_getValues(self, args):
        """Get the value of given paths"""
        if self.checkConnection():
            resp = self.commThread.getValues(args.Path, args.attribute)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getTargetValue)
    def do_getTargetValue(self, args):
        """Get the value of a path"""
        if self.checkConnection():
            resp = self.commThread.getValue(args.Path, "targetValue")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getTargetValues)
    def do_getTargetValues(self, args):
        """Get the value of given paths"""
        if self.checkConnection():
            resp = self.commThread.getValues(args.Path, "targetValue")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_subscribe)
    def do_subscribe(self, args):
        """Subscribe the value of a path"""
        if self.checkConnection():
            if args.output_to_file:
                logPath = pathlib.Path.cwd() / \
                    f"log_{args.Path.replace('/', '.')}_{args.attribute}_{str(time.time())}"
                callback = functools.partial(self.subscribeCallback, logPath)
            else:
                callback = functools.partial(self.subscribeCallback, None)

            resp = self.commThread.subscribe(
                args.Path, callback, args.attribute)
            resJson = json.loads(resp)
            if "subscriptionId" in resJson:
                self.subscribeIds.add(resJson["subscriptionId"])
                if args.output_to_file:
                    logPath.touch()
                    print(f"Subscription log available at {logPath}")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_subscribeMultiple)
    def do_subscribeMultiple(self, args):
        """Subscribe to updates of given paths"""
        if self.checkConnection():
            if args.output_to_file:
                logPath = pathlib.Path.cwd() / \
                    f"subscribeMultiple_{args.attribute}_{str(time.time())}.log"
                callback = functools.partial(self.subscribeCallback, logPath)
            else:
                callback = functools.partial(self.subscribeCallback, None)
            resp = self.commThread.subscribeMultiple(
                args.Path, callback, args.attribute)
            resJson = json.loads(resp)
            if "subscriptionId" in resJson:
                self.subscribeIds.add(resJson["subscriptionId"])
                if args.output_to_file:
                    logPath.touch()
                    print(f"Subscription log available at {logPath}")
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_unsubscribe)
    def do_unsubscribe(self, args):
        """Unsubscribe an existing subscription"""
        if self.checkConnection():
            resp = self.commThread.unsubscribe(args.SubscribeId)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))
            self.subscribeIds.discard(args.SubscribeId)
            self.pathCompletionItems = []

    def stop(self):
        if self.commThread is not None:
            self.commThread.stop()
            self.commThread.join()

    def getMetaData(self, path):
        """Get MetaData of the path"""
        if self.checkConnection():
            return self.commThread.getMetaData(path)
        return "{}"

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_updateVSSTree)
    def do_updateVSSTree(self, args):
        """Update VSS Tree Entry"""
        if self.checkConnection():
            resp = self.commThread.updateVSSTree(args.Json)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_updateMetaData)
    def do_updateMetaData(self, args):
        """Update MetaData of a given path"""
        if self.checkConnection():
            resp = self.commThread.updateMetaData(args.Path, args.Json)
            print(highlight(resp, lexers.JsonLexer(),
                  formatters.TerminalFormatter()))

    @with_category(VSS_COMMANDS)
    @with_argparser(ap_getMetaData)
    def do_getMetaData(self, args):
        """Get MetaData of the path"""
        resp = self.getMetaData(args.Path)
        print(highlight(resp, lexers.JsonLexer(), formatters.TerminalFormatter()))
        self.pathCompletionItems = []

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_disconnect)
    def do_disconnect(self, _args):
        """Disconnect from the VISS/gRPC Server"""
        if hasattr(self, "commThread"):
            if self.commThread is not None:
                self.commThread.stop()
                self.commThread = None

    def checkConnection(self):
        if self.commThread is None or not self.commThread.checkConnection():
            self.connect()
        return self.commThread.checkConnection()

    def connect(self):
        """Connect to the VISS/gRPC Server"""
        if hasattr(self, "commThread"):
            if self.commThread is not None:
                self.commThread.stop()
                self.commThread = None

        # Check we have a valid server URI
        srv = urlparse(self.server)
        config = {"port": 55555, "insecure": True}

        if srv.scheme in ["grpc", "grpcs"]:
            config["protocol"] = "grpc"
        elif srv.scheme in ["ws", "wss"]:
            config["protocol"] = "ws"
            config["port"] = 8090
        else:
            print(f"Invalid server URI. Unsupported protocol: {srv.scheme} ")
            return

        if srv.port is not None:
            config["port"] = srv.port

        if srv.scheme in ["grpcs", "wss"]:
            config["insecure"] = False

        if srv.hostname is None:
            print("No hostname or IP given")
            return

        config["ip"] = srv.hostname

        # Explain were we are connecting to:
        print(f"Connecting to VSS server at {config['ip']} port {config['port']} \
using {'KUKSA GRPC' if config['protocol'] == 'grpc' else 'VISS' } protocol.")
        print(f"TLS will {'not be' if config['insecure'] else 'be'} used.")

        # Configs should only be added if they actually have a value
        if self.token_or_tokenfile:
            config['token_or_tokenfile'] = self.token_or_tokenfile
        if self.certificate:
            config['certificate'] = self.certificate
        if self.keyfile:
            config['keyfile'] = self.keyfile
        if self.cacertificate:
            config['cacertificate'] = self.cacertificate
        if self.tls_server_name:
            config['tls_server_name'] = self.tls_server_name

        self.commThread = KuksaClientThread(config)
        self.commThread.start()

        waitForConnection = threading.Condition()
        waitForConnection.acquire()
        waitForConnection.wait_for(self.commThread.checkConnection, timeout=1)
        waitForConnection.release()

        if self.commThread.checkConnection():
            pass
        else:
            print(
                "Error: Websocket could not be connected or the gRPC channel could not be created.")
            self.commThread.stop()
            self.commThread = None

    @with_category(COMM_SETUP_COMMANDS)
    @with_argparser(ap_connect)
    def do_connect(self, args):
        """Connect to a VSS server"""
        self.server = args.server
        self.connect()

    def getDefaultTokenDir(self):
        try:
            return os.path.join(kuksa_certificates.__certificate_dir__, "jwt")
        except AttributeError:
            guessTokenDir = os.path.join(
                scriptDir, "../kuksa_certificates/jwt")
            if os.path.isdir(guessTokenDir):
                return guessTokenDir
            return "Unknown"

    @with_category(INFO_COMMANDS)
    def do_info(self, _args):
        """Show summary info of the client"""
        print("kuksa-client version " + _metadata.__version__)
        print("Uri: " + _metadata.__uri__)
        print("Author: " + _metadata.__author__)
        print("Copyright: " + _metadata.__copyright__)
        print("Default tokens directory: " + self.getDefaultTokenDir())

    @with_category(INFO_COMMANDS)
    def do_version(self, _args):
        """Show version of the client"""
        print(_metadata.__version__)

    @with_category(INFO_COMMANDS)
    def do_printTokenDir(self, _args):
        """Show default token directory"""
        print(self.getDefaultTokenDir())
# pylint: enable=too-many-public-methods
# pylint: enable=too-many-instance-attributes

# Main Function


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'server', nargs='?', help=f"VSS server to connect to. Format: protocol://host[:port]. \
        Supported protocols: [grpc, grpcs, ws, wss]. Example: {DEFAULT_SERVER_ADDR}", default=DEFAULT_SERVER_ADDR)
    parser.add_argument(
        '--logging-config', default=os.path.join(scriptDir, 'logging.ini'), help="Path to logging configuration file",
    )
    parser.add_argument(
        '--token_or_tokenfile', default=None, help="JWT token or path to a JWT token file (.token)",
    )

    # Add TLS arguments
    # Note: Databroker does not yet support mutual authentication, so no need to use two first arguments
    parser.add_argument(
        '--certificate', default=None, help="Client cert file(.pem), only needed for mutual authentication",
    )
    parser.add_argument(
        '--keyfile', default=None, help="Client private key file (.key), only needed for mutual authentication",
    )
    parser.add_argument(
        '--cacertificate', default=None, help="Client root cert file (.pem). \
        Needed for TLS enabled transports (grpcs, wss)",
    )
    # Observations for Python
    # Connecting to "localhost" works well, subjectAltName seems to suffice
    # Connecting to "127.0.0.1" does not work unless server-name specified
    # For KUKSA.val example certs default name is "Server"
    parser.add_argument(
        '--tls-server-name', default=None,
        help="CA name of server, needed in some cases where subjectAltName does not suffice",
    )

    args = parser.parse_args()

    logging.config.fileConfig(args.logging_config)

    clientApp = TestClient(args.server, token_or_tokenfile=args.token_or_tokenfile,
                           certificate=args.certificate, keyfile=args.keyfile,
                           cacertificate=args.cacertificate, tls_server_name=args.tls_server_name)
    try:
        # We exit the loop when the user types "quit" or hits Ctrl-D.
        clientApp.cmdloop()
    finally:
        clientApp.stop()


if __name__ == "__main__":
    sys.exit(main())
