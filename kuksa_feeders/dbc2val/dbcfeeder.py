#!/usr/bin/env python

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################


import os, sys, signal
import configparser
import queue

import dbc2vssmapper
import dbcreader
import j1939reader
import elm2canbridge

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))
from kuksa_viss_client import KuksaClientThread

print("kuksa.val DBC example feeder")
config_candidates=['/config/dbc_feeder.ini', '/etc/dbc_feeder.ini', os.path.join(scriptDir, 'config/dbc_feeder.ini')]
for candidate in config_candidates:
    if os.path.isfile(candidate):
        configfile=candidate
        break
if configfile is None:
    print("No configuration file found. Exiting")
    sys.exit(-1)
config = configparser.ConfigParser()
config.read(configfile)

kuksaconfig = config
if "kuksa_val" in config:
    kuksaconfig = config["kuksa_val"]
kuksa = KuksaClientThread(kuksaconfig)
kuksa.start()
kuksa.authorize()

mapping = dbc2vssmapper.mapper(kuksaconfig.get('mapping', "mapping.yml"))
canQueue = queue.Queue()

if "can" not in config:
    print("can section missing from configuration, exiting")
    sys.exit(-1)

cancfg = config['can']
canport = cancfg['port']

if config["can"].getboolean("j1939", False):
    print("Use j1939 reader")
    reader = j1939reader.J1939Reader(cancfg,canQueue,mapping)
else:
    print("Use dbc reader")
    reader = dbcreader.DBCReader(cancfg, canQueue,mapping)

if canport == 'elmcan':
    if canport not in config:
        print("section {} missing from configuration, exiting".format(canport))
        sys.exit(-1)

    print("Using elmcan. Trying to set up elm2can bridge")
    elmbr=elm2canbridge.elm2canbridge(canport, config[canport], reader.canidwl)

reader.start_listening()
running = True

def terminationSignalreceived(signalNumber, frame):
    global running, kuksa, reader
    running = False
    kuksa.stop()
    reader.stop()
    print("Received termination signal. Shutting down")

signal.signal(signal.SIGINT, terminationSignalreceived)
signal.signal(signal.SIGQUIT, terminationSignalreceived)
signal.signal(signal.SIGTERM, terminationSignalreceived)

while running:
    try:
        signal, value=canQueue.get(timeout=1)
        for target in mapping[signal]['targets']:
            tv=mapping.transform(signal,target,value)
            if tv is not None: #none indicates the transform decided to not set the value
                print("Update VSS path {} to {} based on signal {}".format(target, tv, signal))
                kuksa.setValue(target, tv)
            pass
    except queue.Empty:
        pass

