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

import os,sys
import serial, can
import threading
from multiprocessing import Queue, Process

# To limit memory in case parsing thread dies and serial reader keeps filling
QUEUE_MAX_ELEMENTS = 2048




class elm2canbridge:
    def __init__(self,cfg):
        print("Try setting up elm2can bridge")
        print("Creating virtual CAN interface")
        os.system("./createelmcanvcan.sh")

        elm = serial.Serial()
        elm.baudrate = cfg['elm.baudrate']
        elm.port = cfg['elm.port']
        elm.timeout = 10
        elm.open()

        if not elm.is_open:
            print("elm2canbridge: Can not open serial port")
            sys.exit(-1)

        self.initelm(elm,cfg['elm.canspeed'], cfg['elm.canack'])
        can = self.initcan(cfg)

        serQueue = Queue(QUEUE_MAX_ELEMENTS)

        mt = threading.Thread(target=self.serialProcesor, args=(serQueue, can,))
        mt.start()

        sr = p = Process(target=self.serialReader, args=(elm, serQueue,))
        #sr = threading.Thread(target=self.serialReader, args=(elm, serQueue,))
        sr.start()
        srpid=sr.pid
        print("Running on pid {}".format(srpid))

    def serialReader(self, elm, q):
        # No time to loose. Read and stuff into queue
        # using bytearray, reading bigger strides and searching for '\r' gets input overruns in UART
        # so this is the dumbest, fastest way

        buffer = bytearray(64)
        index = 0

        os.nice(-10)
        print("elm2canbridge: Enter monitoring mode...")
        elm.write(b'STMA\r')
        elm.read(5)  # Consume echo
        elm.timeout = None
        CR=13
        while True:
            buffer[index] = elm.read()[0]
            #print("Read: {}=={} ".format(buffer[index],CR))
            #print("Buffer {}".format(buffer))
            if buffer[index] == CR or index == 63:
                #print("Received {}".format(bytes(buffer).hex()[:index]))
                q.put(buffer[:index]) #Todo will slice copy deep enough or is this a race?
                index=0
                continue
            index+=1




    def serialProcesor(self,q, candevice):
        print("elm2canbridge: Waiting for incoming...")

        while True:
            line=q.get().decode('utf-8')
            #print("Received {}".format(line))

            isextendedid=False
            #print("Received from elm: {}".format(line))
            try:
                items = line.split()
                if len(items[0]) == 3:  # normal id
                    canid=int(items[0], 16)
                    #print("Normal ID {}".format(canid))
                    del items[0]
                elif len(items) >= 4:  # extended id
                    isextendedid=True
                    canid = int(items[0] + items[1] + items[2] + items[3], 16)
                    items = items[4:]
                    #print("Extended ID {}".format(canid))
                else:
                    print(
                        "Parseline: Invalid line: {}, len first element: {}, total elements: {}".format(line, len(items[0]),
                                                                                                        len(items)))
                    continue

                data=''.join(items)
                #print("data: {}".format(data))
                dataBytes= bytearray.fromhex(data)
            except Exception as e:
                print("Error parsing: " + str(e))
                print("Error. ELM line, items **{}**".format(line.split()))
                continue

            canmsg = can.Message(arbitration_id=canid, data=dataBytes, is_extended_id=isextendedid)
            try:
                candevice.send(canmsg)
            except Exception as e:
                print("Error formwarding message to Can ID 0x{:02x} (extended: {}) with data 0x{}".format(canid, isextendedid, dataBytes.hex()))
                print("Error: {}".format(e))


    #Currently only works with obdlink devices
    def initelm(self, elm, canspeed, ack):
        print("Detecting ELM...")
        elm.write(b'\r\r')
        self.waitforprompt(elm)
        self.writetoelm(elm, b'ATI\r')
        resp = self.readresponse(elm)
        if not resp.strip().startswith("ELM"):
            print("Unexpected response to ATI: {}".format(resp))
            sys.exit(-1)

        self.waitforprompt(elm)
        print("Disable linefeed")
        self.executecommand(elm, b'ATL 0\r')
        print("Enable Headers")
        self.executecommand(elm, b'AT H1\r')
        print("Enable Spaces")
        self.executecommand(elm, b'AT S1\r')
        print("Disable DLC")
        self.executecommand(elm, b'AT D0\r')
        print("Set CAN speed")
        self.executecommand(elm, b'STP 32\r')
        cmd = "STPBR " + str(canspeed) + "\r"
        self.executecommand(elm, cmd.encode('utf-8'))
        baud = self.executecommand(elm, b'STPBRR\r', expectok=False)
        print("Speed is {}".format(canspeed))
        if ack:
            self.executecommand(elm, b'STCMM 1\r')
        else:
            self.executecommand(elm, b'STCMM 0\r')

    #open vcan where we mirror the elmcan monitor output
    def initcan(self, cfg):
        return can.interface.Bus(cfg['can.port'], bustype='socketcan')

    def waitforprompt(self,elm):
        while elm.read() != b'>':
            pass

    def writetoelm(self,elm,data):
        # print("Write")
        length = len(data)
        elm.write(data)
        echo = elm.read(length)
        if echo != data:
            print("elm2canbridge: Not the same {}/{}".format(data, echo))
        # print("Write Done")


    def readresponse(self, elm):
        response=""
        while True:
            d=elm.read()
            if d == b'\r':
                return response
            response=response+d.decode('utf-8')
        #print("DEBUG: "+response)

    def executecommand(self, elm, command, expectok=True):
        self.writetoelm(elm, command)
        resp = self.readresponse(elm)
        if expectok and resp.strip() != "OK":
            print("Invalid response {} for command {}".format(resp, command))
            sys.exit(-1)
        self.waitforprompt(elm)
        return resp
