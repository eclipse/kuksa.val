#! /usr/bin/env python

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
import datetime
import json
import pathlib
import signal
import sys
import threading
import time

import boto3
import kuksa_client
from pyarrow import parquet
import pyarrow as pa
import pyarrow_mapping


scriptDir = pathlib.Path(__file__).resolve().parent

class S3Client():

    # Constructor
    def __init__(self, config):
        print("Init S3 client...")
        if "s3" not in config:
            print("s3 section missing from configuration, exiting")
            sys.exit(-1)
        s3_config = config['s3']
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
        for bucket in response['Buckets']:
            print(f'  {bucket["Name"]}')
            if self.bucket == bucket["Name"]:
                bucket_exists = True
        if not bucket_exists:
            print("create bucket " + self.bucket)
            self.client.create_bucket(Bucket=self.bucket)


    def upload(self, src_file, dst_file):
        # Output the bucket names
        print(f'update file {src_file} to s3://{self.bucket}/{dst_file}')
        self.client.upload_file(src_file, self.bucket, dst_file)
        time.sleep(1)

        # read uploaded file for debugging
        try:
            self.client.download_file(self.bucket, dst_file, src_file + ".downloaded")
        except Exception as exc:
            print("The object does not exist." + str(exc))



class KuksaClient():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        provider_config = config['kuksa_val']
        self.client = kuksa_client.KuksaClientThread(provider_config)
        self.client.start()
        self.client.authorize()
        self.dataset = {}

    def get_metadata(self, path):
        metadata = (json.loads(self.client.getMetaData(path))["metadata"])

        return metadata

    def get_value(self, path):
        dataset = json.loads(self.client.getValue(path))["data"]["dp"]["value"]
        # convert to strings to Arrays, which is required by pyarrow.from_pydict
        if not isinstance(dataset, dict):
            dataset = {path: [dataset]}
        else:
            for key in dataset:
                dataset[key] = [dataset[key]]
        return dataset


    def shutdown(self):
        self.client.stop()


class ParquetPacker():
    def __init__(self, config):
        print("Init parquet packer...")
        if "parquet" not in config:
            print("parquet section missing from configuration, exiting")
            sys.exit(-1)

        self.dataprovider = KuksaClient(config)
        self.uploader = S3Client(config)
        config = config['parquet']
        self.interval = config.get('interval', 1)
        self.paths = config.get('paths', "")
        self.paths = self.paths.replace(" ", "").split(",")
        self.max_num_rows = config.getint('max_num_rows', -1)

        self.schema = self.gen_schema(self.paths)
        self.create_new_parquet()
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def create_new_parquet(self):
        current_time = datetime.datetime.now().strftime("%Y-%b-%d_%H_%M_%S.%f")
        self.pqfile = 'kuksa_' + current_time +'.parquet'
        self.pqwriter = parquet.ParquetWriter(self.pqfile, self.schema)
        self.num_rows = 0


    def gen_fields(self, metadata, prefix=""):
        fields = []
        # Note: only first children is considered, which mapped the vss tree structure
        for key in metadata:
            #print("key is " + key)
            if "children" in metadata[key]:
                prefix += key + "."
                return pa.struct(self.gen_fields(metadata[key]["children"], prefix))
            #print("prefix is " + prefix)
            datatype = pyarrow_mapping.KUKSA_TO_PYARROW_MAPPING[metadata[key]["datatype"]]()
            #print("datatype is " + str(datatype))
            fields.append(pa.field(prefix+key, datatype))


        return fields

    def gen_schema(self, paths):
        fields = [pa.field("ts", pa.timestamp('us'))]
        for path in paths:
            metadata = self.dataprovider.get_metadata(path)
            fields += (self.gen_fields(metadata))

        return pa.schema(fields)


    def write_table(self, data):
        table = pa.Table.from_pydict(data)
        table = table.add_column(0, "ts", [[datetime.datetime.now()]])
        try:
            table = table.cast(self.schema)
            self.pqwriter.write_table(table)
            self.num_rows += 1
        except pa.lib.ArrowInvalid as exc:
            print("ERROR: " + str(exc))
            print("The following data can not be written into parquet file:")
            print(data)

        if self.max_num_rows > 0 and self.num_rows >= self.max_num_rows:
            self.pqwriter.close()
            self.uploader.upload(self.pqfile, self.pqfile )
            self.create_new_parquet()


    def loop(self):
        print("receive loop started")
        while self.running:

            data = {}
            for path in self.paths:
                data.update(self.dataprovider.get_value(path))

            self.write_table(data)

            time.sleep(int(self.interval))

    def shutdown(self):

        self.running = False
        self.dataprovider.shutdown()
        self.pqwriter.close()
        self.thread.join()


def main():
    config_path = scriptDir / 'config.ini'
    if not config_path.is_file():
        print(f"Could not find configuration file: {config_path}. Exiting.")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(config_path)

    client = ParquetPacker(config)

    def handle_termination(_signum, _frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, handle_termination)
    signal.signal(signal.SIGQUIT, handle_termination)
    signal.signal(signal.SIGTERM, handle_termination)

if __name__ == "__main__":
    sys.exit(main())
