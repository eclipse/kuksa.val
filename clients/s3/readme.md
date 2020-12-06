# S3 uploader
This client receive data from kuksa_val and pack the configured data into [parquet format](https://parquet.apache.org/documentation/latest/) and upload it to a s3 server

Check [config.ini](./config.ini) file for the configuration of this client.

## Parquet file
You can use the python command line tool [parquet-tools](https://pypi.org/project/parquet-tools/) to view generated parquet files:

```
    parquet-tools show s3://bucket-name/prefix/*
    parquet-tools show kuksa.parquet

```
