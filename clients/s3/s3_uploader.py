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


import threading
import configparser
import os, sys, json, signal
import csv
import time
import queue
from pyarrow import parquet
from pyarrow import fs
import boto3
import pyarrow as pa
import numpy as np 
import awswrangler as wr
import pandas as pd
from datetime import date


scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../common/"))
from clientComm import VSSClientComm

class S3_Client():

    # Constructor
    def __init__(self, config):
        print("Init S3 client...")
        if "s3" not in config:
            print("s3 section missing from configuration, exiting")
            sys.exit(-1)
        s3_config=config['s3']
        self.client = boto3.client('s3',
            endpoint_url="http://localhost:4566",
            aws_access_key_id='accesskey',
            aws_secret_access_key='secretkey',
            verify=False)
        bucket = s3_config.get('bucket', "testbucket")
        self.client.create_bucket(Bucket= bucket)
        response = self.client.list_buckets()

        # Output the bucket names
        s3 = fs.S3FileSystem(endpoint_override='http://localhost:4566')
        table = pa.table({'col1': range(3), 'col2': np.random.randn(3)})
        print(table.schema)
        parquet.write_table(table, "test.parquet")
        self.client.upload_file("test.parquet", bucket,"test")
        #parquet.write_table(table, "test/data.parquet", filesystem=s3)
        f = s3.open_input_stream(f'{bucket}/test')
        print(parquet.read_table(f'{bucket}/test', filesystem=s3))
        with s3.open_output_stream(f'{bucket}/test2') as file:
           with parquet.ParquetWriter(file, table.schema) as writer:
              writer.write_table(table)
        print('Existing buckets:')
        for b in response['Buckets']:
            print(f'  {b["Name"]}')

        #df = pd.DataFrame({
        #    "id": [3],
        #    "value": ["bar"],
        #    "date": [date(2020, 1, 3)]
        #})

        #wr.s3.to_parquet(
        #    df=df,
        #    path="test.parquet",
        #    dataset=True,
        #    mode="append"
        #)



class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        provider_config=config['kuksa_val']
        self.sendMsgQueue = queue.Queue()
        self.recvMsgQueue = queue.Queue()
        self.client = VSSClientComm(self.sendMsgQueue, self.recvMsgQueue, provider_config)
        self.client.start()
        self.token = provider_config.get('token', "token.json")
        self.authorize()
        
    def authorize(self):
        if os.path.isfile(self.token):
            with open(self.token, "r") as f:
                self.token = f.readline()

        req = {}
        req["requestId"] = 1238
        req["action"]= "authorize"
        req["tokens"] = self.token
        jsonDump = json.dumps(req)
        self.sendMsgQueue.put(jsonDump)
        #print(req)
        resp = self.recvMsgQueue.get(timeout = 1)
        #print(resp)

    def shutdown(self):
        self.client.stopComm()

    def setValue(self, path, value):
        if 'nan' == value:
            print(path + " has an invalid value " + str(value))
            return
        req = {}
        req["requestId"] = 1235
        req["action"]= "set"
        req["path"] = path
        req["value"] = value
        jsonDump = json.dumps(req)
        #print(jsonDump)
        self.sendMsgQueue.put(jsonDump)
        resp = self.recvMsgQueue.get(timeout = 1)
        #print(resp)
        
    def setPosition(self, position):
        self.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Altitude', position['alt'])
        self.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Latitude', position["lat"])
        self.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Longitude', position["lon"])
        self.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Accuracy', position["hdop"])
        self.setValue('Vehicle.Cabin.Infotainment.Navigation.CurrentLocation.Speed', position["speed"])

class Parquet_Packer():
    def __init__(self, config, consumer):
        print("Init parquet packer...")
        if "parquet" not in config:
            print("parquet section missing from configuration, exiting")
            sys.exit(-1)
        
        self.consumer = consumer
        provider_config=config['gpsd']
        gpsd_host=provider_config.get('host','127.0.0.1')
        gpsd_port=provider_config.get('port','2947')

        print("Trying to connect gpsd at "+str(gpsd_host)+" port "+str(gpsd_port))


        self.position = {"lat":"0", "lon":"0", "alt":"0", "hdop":"0", "speed":"0" }
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=(gpsd,))
        self.thread.start()

    def loop(self, gpsd):
        print("gpsd receive loop started")
        while self.running:
            try:
                df = pd.DataFrame({'one': [-1, np.nan, 2.5],

                   'two': ['foo', 'bar', 'baz'],

                   'three': [True, False, True]},

                   index=list('abc'))
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
                self.shutdown()
                return

            self.consumer.setPosition(self.position)
     
            time.sleep(1) 


    def shutdown(self):
        self.running=False
        self.consumer.shutdown()
        self.thread.join()

        
if __name__ == "__main__":
    config_candidates=['config.ini']
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile=candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)
    
    client = S3_Client(config)

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

