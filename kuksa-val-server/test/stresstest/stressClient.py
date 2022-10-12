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

import configparser
import sys
import argparse
import time
from pickle import FALSE

from kuksa_client import *

class StressClient():

    appClient = {}
    cfg = {}

    def getConfig(self):

        config = configparser.ConfigParser()
        config.read('config.ini')

        vsscfg = config['vss']      #get data from config file

        return vsscfg

    def connect(self,cfg):
        """Connect to the VISS Server"""
        if hasattr(self, "commThread"):
            if self.commThread != None:
                self.commThread.stop()
                self.commThread = None
        self.commThread = KuksaClientThread(self.cfg)       #make new thread
        self.commThread.start()                             #start thread

    def __init__(self):
        self.cfg = self.getConfig()
        try:
            self.connect(self.cfg)
            self.commThread.timeout = self.cfg.getfloat("timeout", fallback=0.1)                       #get necessary timeout for stresstest
            self.commThread.authorize(token=self.commThread.tokenfile)
        except:
            print("Could not connect successfully")
            sys.exit(-1)
