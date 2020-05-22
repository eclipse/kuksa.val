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
        mt = threading.Thread(target=self.monitoringThread, args=(elm, can,))
        mt.start()

    def monitoringThread(self,elm, can):
        print("elm2canbridge: Enter monitoring mode...")
        elm.write(b'STMA\r')
        elm.read(5)  # Consume echo
        elm.timeout = None

        # the builtin readline in python serial lib is too slow
        rlh = SerReadLineHelper(elm)
        while True:
            line = rlh.readline()
            self.forward(line, can)

    #parse line from ELM and  create a real CAN message
    def forward(self,elmrx,candevice):
        line=elmrx.decode('utf-8')
        isextendedid=False
        #print("Received from elm: {}".format(line))
        try:
            items = line.split()
            if len(items[0]) == 3:  # nromal id
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
                return

            data=''.join(items)
            #print("data: {}".format(data))
            dataBytes= bytearray.fromhex(data)
        except Exception as e:
            print("Error parsing: " + str(e))
            print("Error. ELM line //{}//, items **{}**".format(elmrx.hex(), line.split()))
            return

        canmsg = can.Message(arbitration_id=canid, data=dataBytes, is_extended_id=isextendedid)
        candevice.send(canmsg)



    #Currently only works with obdlink devices
    def initelm(self, elm, canspeed, ack):
        print("Detecting ELM...")
        elm.write(b'\r\r')
        self.waitforprompt(elm)
        self.writetoelm(elm, b'ATI\r')
        resp = self.readresponse(elm)
        if not resp.startswith("ELM"):
            print("Unexpected response to ATI: {}".format(resp))
            sys.exit(-1)

        self.waitforprompt(elm)
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


class SerReadLineHelper:
    def __init__(self, s):
        self.buf = bytearray()
        self.s = s

    def readline(self):
        i = self.buf.find(b"\r")
        if i >= 0:
            r = self.buf[:i + 1]
            self.buf = self.buf[i + 1:]
            return r
        while True:
            i = max(1, min(2048, self.s.in_waiting))
            data = self.s.read(i)
            i = data.find(b"\r")
            if i >= 0:
                r = self.buf + data[:i + 1]
                self.buf[0:] = data[i + 1:]
                return r
            else:
                self.buf.extend(data)
