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
from datetime import datetime
import pyarrow_mapping


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
        parquet.write_table(table, "test.parquet", coerce_timestamps='ms', allow_truncated_timestamps = True)
        parquet.write_to_dataset(table, root_path="test", coerce_timestamps='ms', allow_truncated_timestamps=True)
        #self.client.upload_file("test.parquet", bucket,"test")
        #parquet.write_table(table, "test/data.parquet", filesystem=s3)
        #print(parquet.read_table(f'{bucket}/test', filesystem=s3))
        with s3.open_output_stream(f'{bucket}/test2') as file:
           with parquet.ParquetWriter(file, table.schema) as writer:
              writer.write_table(table)
              #writer.write_to_dataset(table)
        f = s3.open_input_stream(f'{bucket}/test2')
        print(f.readall())
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
        self.client.authorize(self.token)
        self.dataset = {}
        
    def getMetaData(self, path):
        return json.loads(self.client.getMetaData(path))["metadata"]

    def getValue(self, path):
        dataset = json.loads(self.client.getValue(path))["value"]
        if not isinstance(dataset, dict):
            return {path: [dataset]}
        return dataset

    def shutdown(self):
        self.client.stopComm()

        
class Parquet_Packer():
    def __init__(self, config, dataprovider):
        print("Init parquet packer...")
        if "parquet" not in config:
            print("parquet section missing from configuration, exiting")
            sys.exit(-1)
        
        self.provider = dataprovider
        config=config['parquet']
        self.path = config.get('path', "")
        self.interval = config.getint('interval', 1)
        
        self.schema = self.genSchema(self.provider.getMetaData(self.path))
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def get_childtree(self, vssTree, pathText):
        childVssTree = vssTree
        if "." in pathText:
            paths = pathText.split(".")
            for path in paths:
                if path in childVssTree:
                    childVssTree = childVssTree[path]
                elif 'children' in childVssTree and path in childVssTree['children']:
                    childVssTree = childVssTree['children'][path]
            if 'children' in childVssTree:
                childVssTree = childVssTree['children']
        return childVssTree

    def genSchema(self, metadata, prefix = ""):
        fields = [pa.field("timestamp", pa.timestamp('us'))]
        # TODO only first considered
        for key in metadata:
            #print("key is " + key)
            if "children" in metadata[key]:
                prefix += key + "."
                return self.genSchema(metadata[key]["children"], prefix)
            else:
                #print("prefix is " + prefix)
                datatype = pyarrow_mapping.KUKSA_TO_PYARROW_MAPPING[metadata[key]["datatype"]]()
                #print("datatype is " + str(datatype))
                fields.append(pa.field(prefix+key, datatype))
            

        return pa.schema(fields)




    def loop(self):
        print("receive loop started")
        while self.running:
            time.sleep(self.interval) 
            break

        data=self.provider.getValue(self.path)
        print(data)
        table = pa.Table.from_pydict(data)
        table = table.add_column(0, "timestamp", [[datetime.now()]])
        print(table.columns)
        table = table.cast(self.schema)
        try:
            table_origin = parquet.read_table(self.path+'.parquet')
            table = pa.concat_tables([table_origin, table])
        except FileNotFoundError:
            pass
        print(table.columns)
        print(table)
        parquet.write_table(table,self.path+'.parquet', coerce_timestamps='us', allow_truncated_timestamps = True)


    def shutdown(self):
        self.running=False
        self.provider.shutdown()
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
    
    packer = Parquet_Packer(config, Kuksa_Client(config))
    #client = S3_Client(config)

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        packer.shutdown()
        #client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

