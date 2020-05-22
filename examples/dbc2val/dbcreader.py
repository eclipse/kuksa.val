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

import can, cantools
import threading

import time

class DBCReader:
    def __init__(self, cfg, rxqueue, mapper):
        self.queue=rxqueue
        self.mapper=mapper
        print("Reading dbc file")
        self.db = cantools.database.load_file(cfg['vss.dbcfile'])

        self.parseErr=0
        print("Open CAN device {}".format(cfg['can.port']))
        self.bus = can.interface.Bus(cfg['can.port'], bustype='socketcan')
        rxThread = threading.Thread(target=self.rxWorker)
        rxThread.start()

    def rxWorker(self):
        print("Starting thread")
        while True:
            msg=self.bus.recv()
            try:
                decode=self.db.decode_message(msg.arbitration_id, msg.data)
                #print("Decod" +str(decode))
            except:
                self.parseErr+=1
                #print("Error Decoding")
                continue
            rxTime=time.time()
            for k,v in decode.items():
                if k in self.mapper:
                    if self.mapper.minUpdateTimeElapsed(k, rxTime):
                        self.queue.put((k,v))




