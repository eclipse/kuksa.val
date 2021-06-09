import configparser
import sys
import ssl
import argparse
import time
from pickle import FALSE

from kuksa_viss_client import *

class StressClient():

    appClient = {}
    cfg = {}


    def getConfig(self):

        config = configparser.ConfigParser()
        config.read('config.ini')

        vsscfg = config['vss']      #get data from config file
        """self.cfg['ip'] = vsscfg.get("ip")
        self.cfg['port'] = vsscfg.get("port","8090")
        self.cfg['insecure'] = vsscfg.get("insecure",FALSE)
        self.cfg['token'] = vsscfg.get("token", "jwt.token")
        self.cfg['timeout'] = vsscfg.getfloat("timeout", 0.1)
        self.cfg['certificate'] = vsscfg.get("certificate", "../../kuksa_certificates/Client.pem")
        self.cfg['cacertificate'] = vsscfg.get("cacertificate", "../../kuksa_certificates/CA.pem")
        self.cfg['key'] = vsscfg.get("key", "../../kuksa_certificates/Server.key")"""

        return vsscfg


    def connect(self,cfg):
        """Connect to the VISS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
        self.commThread = KuksaClientThread(self.cfg)       #make new thread
        self.commThread.start()                             #start thread

        pollIndex = 30
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

    def __init__(self):
        cfg = self.getConfig()
        try:
            self.connect(self.cfg)
            self.commThread.timeout = cfg.getfloat("timeout", 0.1)                       #get necessary timeout for stresstest
            self.commThread.authorize(token=self.commThread.tokenfile)
        except:
            print("Could not connect successfully, try using insecure mode on your server")
            sys.exit(-1)
