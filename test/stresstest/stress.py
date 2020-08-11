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
import time

import websocketconnector


cfg = {}


def getConfig():
    global cfg

    parser = argparse.ArgumentParser()
    


    parser.add_argument("-s", "--server", help="VSS server", type=str)
    parser.add_argument("-j", "--jwt", help="JWT security token file", type=str)
    parser.add_argument("-t", "--timeout", help="Timeout between requests", type=float)


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

    cfg['timeout'] = vsscfg.getfloat("timeout", 0.1)
    if args.timeout:
        cfg['timeout'] = args.timeout


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


print("kuksa.val Stresstest")
getConfig()

print("VSS server           : {}".format(cfg['vss.server']))
print("JWT token file       : {}".format(cfg['vss.jwttoken']))
print("Timeout [s]          : {}".format(cfg['timeout']))


with open(cfg['vss.jwttoken'], 'r') as f:
    token = f.read()


vss = websocketconnector.vssclient(cfg['vss.server'], token)


count=0
while True:
    vss.push
    vss.push("Vehicle.OBD.EngineLoad", (count%110)-10)
    vss.push("Vehicle.Speed", count)
    vss.push("Vehicle.Cabin.Door.Row1.Right.Shade.Switch", "Open")


    count+=1
    time.sleep(cfg['timeout'])


sys.exit(0)
