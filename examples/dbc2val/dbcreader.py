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
        self.cfg=cfg
        print("Reading dbc file")
        self.db = cantools.database.load_file(cfg['vss.dbcfile'])

        self.canidwl = self.get_whitelist()

        self.parseErr=0



    def start_listening(self):
        print("Open CAN device {}".format(self.cfg['can.port']))
        self.bus = can.interface.Bus(self.cfg['can.port'], bustype='socketcan')
        rxThread = threading.Thread(target=self.rxWorker)
        rxThread.start()

    def get_whitelist(self):
        print("Collecting signals, generating CAN ID whitelist")
        wl = []
        for entry in self.mapper.map():
            canid=self.get_canid_for_signal(entry[0])
            if canid != None and canid not in wl:
                wl.append(canid)
        return wl

    def get_canid_for_signal(self, sig_to_find):
        for msg in self.db.messages:
            for signal in msg.signals:
                if signal.name == sig_to_find:
                    id = msg.frame_id
                    print("Found signal {} in CAN frame id 0x{:02x}".format(signal.name, id))
                    return id
        print("Signal {} not found in DBC file".format(sig_to_find))
        return None


    def rxWorker(self):
        print("Starting thread")
        while True:
            msg=self.bus.recv()
            try:
                decode=self.db.decode_message(msg.arbitration_id, msg.data)
                #print("Decod" +str(decode))
            except Exception as e:
                self.parseErr+=1
                #print("Error Decoding: "+str(e))
                continue
            rxTime=time.time()
            for k,v in decode.items():
                if k in self.mapper:
                    if self.mapper.minUpdateTimeElapsed(k, rxTime):
                        self.queue.put((k,v))




