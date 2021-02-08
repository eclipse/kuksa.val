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
# This provider gets data from a simple log file which contains lines
# formatted 
# lat,lon
# All other values will be reported as 0


import threading
import configparser
import os, sys, json, signal
import csv
import time
import queue
import gps

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../common/"))
from clientComm import VSSClientComm

class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        provider_config=config['kuksa_val']
        self.client = VSSClientComm(provider_config)
        self.client.start()
        self.token = provider_config.get('token', "token.json")
        self.client.authorize(self.token)
        
    def shutdown(self):
        self.client.stopComm()

    def setPosition(self, position):
        self.client.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Altitude', position['alt'])
        self.client.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Latitude', position["lat"])
        self.client.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Longitude', position["lon"])
        self.client.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Accuracy', position["hdop"])
        self.client.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Speed', position["speed"])

class GPSD_Client():
    def __init__(self, config, consumer):
        print("Init gpsd client...")
        if "gpsd" not in config:
            print("gpsd section missing from configuration, exiting")
            sys.exit(-1)
        
        self.consumer = consumer
        provider_config=config['gpsd']
        gpsd_host=provider_config.get('host','127.0.0.1')
        gpsd_port=provider_config.get('port','2947')

        gpsd = gps.gps(host = gpsd_host, port = gpsd_port, mode=gps.WATCH_ENABLE|gps.WATCH_NEWSTYLE) 
        print("Trying to connect gpsd at "+str(gpsd_host)+" port "+str(gpsd_port))


        self.position = {"lat":"0", "lon":"0", "alt":"0", "hdop":"0", "speed":"0" }
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=(gpsd,))
        self.thread.start()

    def loop(self, gpsd):
        print("gpsd receive loop started")
        while self.running:
            try:
                report = gpsd.next()
                if report['class'] == 'TPV':

                    self.position['lat']= getattr(report,'lat',0.0)
                    self.position['lon']= getattr(report,'lon',0.0)
                    self.position['alt']= getattr(report,'alt', 'nan')
                    self.position['speed']= getattr(report,'speed',0.0)
                    print(getattr(report,'time', "-"))
                    print(self.position)
                
            except Exception as e:
                print("Get exceptions: ")
                print(e)
                time.sleep(1) 
                continue

            self.consumer.setPosition(self.position)
     
            time.sleep(1) 


    def shutdown(self):
        self.running=False
        self.consumer.shutdown()
        self.thread.join()

        
if __name__ == "__main__":
    config_candidates=['gpsd_feeder.ini', '/etc/gpsd_feeder.ini']
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile=candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)
    
    client = GPSD_Client(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

