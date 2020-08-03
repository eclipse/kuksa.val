#!/usr/bin/python3

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################


import sys
import argparse
import configparser
import queue

import dbc2vssmapper
import dbcreader
import websocketconnector
import elm2canbridge


cfg = {}


def getConfig():
    global cfg

    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--device", help="CAN port. Choose \"elmcsn\" for ELM adapter", type=str)

    parser.add_argument("--obdbaudrate", help="Baudrate to ELM if CAN port is \"elmcan\"", type=int)
    parser.add_argument("--obdcanspeed", help="CAN bus speed if CAN port is \"elmcan\"", type=int)
    parser.add_argument("--noobdcanack",
                        help="Do not acknowledge CAN messages be  Only if CAN port is \"elmcan\"",  dest='nocanack_override', action='store_true')
    parser.add_argument("--obdcanack",
                        help="Acknowledge CAN messages be  Only if CAN port is \"elmcan\"",  dest='canack_override', action='store_true')



    parser.add_argument("--obdport", help="Serial port where ELM is connected. Only if CAN port is \"elmcan\"",
                        type=str)

    parser.add_argument("--dbc", help="DBC file used to parse CAN messages", type=str)

    parser.add_argument("-s", "--server", help="VSS server", type=str)
    parser.add_argument("-j", "--jwt", help="JWT security token file", type=str)
    parser.add_argument("--mapping", help="VSS mapping file", type=str)

    args = parser.parse_args()

    config = configparser.ConfigParser()
    config.read('config.ini')

    vsscfg = config['vss']
    cfg['vss.server'] = vsscfg.get("server", "ws://localhost:8090")
    if args.server:
        cfg['vss.server'] = args.server

    cfg['vss.jwttoken'] = vsscfg.get("jwttoken", "jwt.token")
    if args.jwt:
        cfg['vss.jwttoken'] = args.jwt

    cfg['vss.mapping'] = vsscfg.get("mapping", "mapping.yaml")
    if args.mapping:
        cfg['vss.server'] = args.mapping

    cfg['vss.dbcfile'] = vsscfg.get("dbcfile", "example.dbc")
    if args.dbc:
        cfg['vss.dbcfile'] = args.dbc

    cancfg = config['can']
    cfg['can.port'] = cancfg.get("port", "can0")
    if args.device:
        cfg['can.port'] = args.device

    elmcfg = config['elmcan']
    cfg['elm.port'] = elmcfg.get("port", "/dev/ttyS0")
    if args.obdport:
        cfg['elm.port'] = args.obdport

    cfg['elm.baudrate'] = elmcfg.getint("baudrate", 2000000)
    if args.obdbaudrate:
        cfg['elm.baudrate'] = args.obdbaudrate

    cfg['elm.canspeed'] = elmcfg.getint("speed", 500000)
    if args.obdcanspeed:
        cfg['elm.canspeed'] = args.obdcanspeed

    cfg['elm.canack'] = elmcfg.getboolean("canack", False)

    # Can override CAN ACK setting from commandline. Safe choice, no ack, is dominant
    if args.canack_override:
        cfg['elm.canack'] = True
    if args.nocanack_override:
        cfg['elm.canack'] = False


def publishData(vss):
    print("Publish data")
    for obdval, config in mapping.map():

        if config['value'] is None:
            continue
        print("Publish {}: to ".format(obdval), end='')
        for path in config['targets']:
            vss.push(path, config['value'].magnitude)
            print(path, end=' ')
        print("")


print("kuksa.val DBC example feeder")
getConfig()

print("VSS server           : {}".format(cfg['vss.server']))
print("JWT token file       : {}".format(cfg['vss.jwttoken']))
print("DBC  file            : {}".format(cfg['vss.dbcfile']))
print("VSS mapping file     : {}".format(cfg['vss.mapping']))
print("CAN port             : {}".format(cfg['can.port']))
if cfg['can.port'] == "elmcan":
    print("ELM serial port     : {}".format(cfg['elm.port']))
    print("ELM serial baudrate : {}".format(cfg['elm.baudrate']))
    print("ELM CAN Speed       : {}".format(cfg['elm.canspeed']))
    print("ELM Ack CAN frames  : {}".format(cfg['elm.canack']))


with open(cfg['vss.jwttoken'], 'r') as f:
    token = f.read()

mapping = dbc2vssmapper.mapper("mapping.yml")

vss = websocketconnector.vssclient(cfg['vss.server'], token)
canQueue = queue.Queue()

dbcR = dbcreader.DBCReader(cfg,canQueue,mapping)

if cfg['can.port'] == 'elmcan':
    print("Using elmcan. Trying to set up elm2can bridge")
    elmbr=elm2canbridge.elm2canbridge(cfg, dbcR.canidwl)

dbcR.start_listening()

while True:
    signal, value=canQueue.get()
    print("Update signal {} to {}".format(signal, value))
    for target in mapping[signal]['targets']:
        #print("Publish {} to {}".format(signal,target))
        vss.push(target, value)


sys.exit(0)
