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
        endpoint_url = s3_config.get("endpoint_url", None)
        access_key = s3_config.get("access_key_id", None)
        secret_key = s3_config.get("secret_access_key", None)
        verify = s3_config.getboolean("verify", False)
        region_name = s3_config.get("region_name", None)
        api_version = s3_config.get("api_version", None)
        use_ssl = s3_config.getboolean("use_ssl", True)
        token = s3_config.get("session_token", None)
        self.client = boto3.client('s3',
            endpoint_url = endpoint_url,
            aws_access_key_id = access_key,
            aws_secret_access_key = secret_key,
            verify = verify,
            region_name = region_name,
            api_version = api_version,
            use_ssl = use_ssl,
            aws_session_token = token
            )

        # Create bucket if does not exist
        self.bucket = s3_config.get('bucket', "testbucket")
        bucket_exists = False
        response = self.client.list_buckets()
        print('Existing buckets:')
        for b in response['Buckets']:
            print(f'  {b["Name"]}')
            if self.bucket == b["Name"]:
                bucket_exists = True
        if not bucket_exists:
            print("create bucket " + self.bucket)
            self.client.create_bucket(Bucket= self.bucket)


    def upload(self, srcFile, dstFile):

        # Output the bucket names
        self.client.upload_file(srcFile, self.bucket, dstFile)

        # read uploaded file for debugging
        print(f'update file {srcFile} to s3://{self.bucket}/{dstFile}')
        try:
            self.client.download_file(self.bucket, dstFile, srcFile + ".downloaded")
        except Exception as e:
            print("The object does not exist." + str(e))



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
        metadata = (json.loads(self.client.getMetaData(path))["metadata"])
        #print(metadata)

        return metadata

    def getValue(self, path):
        dataset = json.loads(self.client.getValue(path))["value"]
        # convert to strings to Arrays, which is required by pyarrow.from_pydict
        if not isinstance(dataset, dict):
            dataset= {path: [dataset]}
        else:
            for key in dataset:
                dataset[key] = [dataset[key]]
        return (dataset)

    def shutdown(self):
        self.client.stopComm()

        
class Parquet_Packer():
    def __init__(self, config):
        print("Init parquet packer...")
        if "parquet" not in config:
            print("parquet section missing from configuration, exiting")
            sys.exit(-1)
        
        self.dataprovider = Kuksa_Client(config)
        self.uploader = S3_Client(config)
        config=config['parquet']
        self.path = config.get('path', "")
        self.path = self.path.replace(" ", "").split(",")
        self.interval = config.getint('sample_interval', 1)
        self.max_num_rows = config.getint('max_num_rows', -1)
        
        self.schema = self.genSchema(self.path)
        self.createNewParquet()
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def createNewParquet(self):
        currTime = datetime.now().strftime("%Y-%b-%d_%H_%M_%S.%f")
        self.pqfile = 'kuksa_' + currTime +'.parquet'
        self.pqwriter = parquet.ParquetWriter(self.pqfile, self.schema)
        self.num_rows = 0


    def genFields(self, metadata, prefix = ""):
        fields = []
        # Note: only first children is considered, which mapped the vss tree structure
        for key in metadata:
            #print("key is " + key)
            if "children" in metadata[key]:
                prefix += key + "."
                return self.genFields(metadata[key]["children"], prefix)
            else:
                #print("prefix is " + prefix)
                datatype = pyarrow_mapping.KUKSA_TO_PYARROW_MAPPING[metadata[key]["datatype"]]()
                #print("datatype is " + str(datatype))
                fields.append(pa.field(prefix+key, datatype))
            

        return fields

    def genSchema(self, path):
        fields = [pa.field("timestamp", pa.timestamp('us'))]
        for p in path:
            metadata = self.dataprovider.getMetaData(p)
            fields += (self.genFields(metadata))

        
        return pa.schema(fields)


    def writeTable(self, data):
        table = pa.Table.from_pydict(data)
        table = table.add_column(0, "timestamp", [[datetime.now()]])
        try:
            table = table.cast(self.schema)
            self.pqwriter.write_table(table)
            self.num_rows += 1
        except pa.lib.ArrowInvalid as e:
            print("ERROR: " + str(e))
            print("The following data can not be written into parquet file:")
            print(data)

        if self.max_num_rows > 0 and self.num_rows >= self.max_num_rows:
            self.pqwriter.close()
            self.uploader.upload(self.pqfile, self.pqfile )
            self.createNewParquet()


    def loop(self):
        print("receive loop started")
        while self.running:

            data = {}
            for p in self.path:
                data.update(self.dataprovider.getValue(p))

            self.writeTable(data)
            
            time.sleep(self.interval) 

    def shutdown(self):

        self.running=False
        self.dataprovider.shutdown()
        self.pqwriter.close()
        self.thread.join()

        
def main():
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
    
    client = Parquet_Packer(config)

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

if __name__ == "__main__":
    sys.exit(main())
