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

import yaml 

class mapper:

    def __init__(self,input):
        with open(input,'r') as file:
            self.mapping = yaml.full_load(file)
        for key in self.mapping.keys():
            self.mapping[key]['lastupdate']=0.0
            if 'minupdatedelay' not in self.mapping[key]:
                print("Mapper: No minimal update delay defined for signal {}, setting to 1000ms.".format(key))

    def map(self):
        return self.mapping.items()

    #returns true if element can be updated
    def minUpdateTimeElapsed(self,key, time):
        diff=time - self.mapping[key]['lastupdate']
        #print("Curr update {}, last update {}, diff: {}".format(time, self.mapping[key]['lastupdate'], diff ))
        if diff*1000 >= self.mapping[key]['minupdatedelay']:
            self.mapping[key]['lastupdate']=time
            return True
        return False
        #

    def __contains__(self,key):
        return key in self.mapping.keys()

    def __getitem__(self, item):
        return self.mapping[item]
    