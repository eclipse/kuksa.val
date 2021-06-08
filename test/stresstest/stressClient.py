import configparser
import argparse
import time
from pickle import FALSE

from kuksa_viss_client import *

class StressClient():

    appClient = {}
    cfg = {}


    def getConfig(self):

        parser = argparse.ArgumentParser()

        parser.add_argument("-s", "--server", help="VSS server", type=str)
        parser.add_argument("-j", "--jwt", help="JWT security token file", type=str)
        parser.add_argument("-t", "--timeout", help="Timeout between requests", type=float)

        args = parser.parse_args()

        config = configparser.ConfigParser()
        config.read('config.ini')

        vsscfg = config['vss']      #get data from config file
        self.cfg['ip'] = vsscfg.get("ip")

        self.cfg['port'] = vsscfg.get("port","8090")
        
        self.cfg['insecure'] = vsscfg.get("insecure",FALSE)

        self.cfg['token'] = vsscfg.get("token", "jwt.token")

        self.cfg['timeout'] = vsscfg.getfloat("timeout", 0.1)

    def connect(self,cfg):
        insecure = cfg['insecure']
        """Connect to the VISS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
        config = {'ip':self.cfg['ip'],
        'port': self.cfg['port'],
        'insecure' : self.cfg['insecure']
        }
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
        self.getConfig()
        self.connect(self.cfg)
        self.commThread.authorize(token=self.cfg['token'],timeout=self.cfg['timeout'])