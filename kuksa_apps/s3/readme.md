# S3 uploader

This client periodically gets data from either kuksa_val_server or kuksa_databroker, packs it using [parquet format](https://parquet.apache.org/documentation/latest/) and upload it to a S3 server.

Check [config.ini](./config.ini) file for the configuration of this client.

## Install

To install S3 uploader python dependencies, run:
```console
$ pip install -r requirements.txt
```

You may also want to [install AWS CLI](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html)
to interact with an actual S3 instance or a locally hosted one.

## Test locally

Default configuration allows you to test S3 uploader against a locally hosted S3 server.

To start a local S3 server in the background, run:
```console
$ docker run -d -p 8000:8000 --name cloudserver zenko/cloudserver:latest
```

Now, you need to create the `kuksa` bucket using `AWS CLI`:
```console
$ aws s3 mb s3://kuksa --endpoint-url http://localhost:8000
```

Default configuration will run against `kuksa_val_server`.
To start it in the background, run:
```console
$ docker run -d -p 127.0.0.1:8090:8090 -v $HOME/kuksaval.config:/config -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/kuksa-val:latest
```

Then you need to set some initial values for default paths:
```console
$ kuksa_client
[...]
Test Client> authorize ../../kuksa_certificates/jwt/all-read-write.json.token
{
  "TTL": 1767225599,
  "action": "authorize",
  "requestId": "725a9468-4ae7-4674-86d2-45dd2b0a11ec",
  "ts": "2022-12-07T07:24:44.1670399804Z"
}
Test Client> setValue Vehicle.Speed 12
{
  "action": "set",
  "requestId": "4a84682b-51d8-407e-931a-36edec395cf0",
  "ts": "2022-12-07T07:24:53.1670397893Z"
}

Test Client> setValue Vehicle/Cabin/Infotainment/Navigation/DestinationSet/Latitude 52
{
  "action": "set",
  "requestId": "629d85ba-67c1-44c5-b2f9-cd92aa20b377",
  "ts": "2022-12-07T07:24:57.1670397897Z"
}

Test Client> setValue Vehicle/Cabin/Infotainment/Navigation/DestinationSet/Longitude 60
{
  "action": "set",
  "requestId": "10249a5a-a936-45c6-848e-c0551e4eda53",
  "ts": "2022-12-07T07:25:02.1670397902Z"
}
```

You may now run S3 uploader, wait for some seconds until you see `update file kuksa[...].parquet to s3://kuksa/kuksa[...].parquet`:

```console
$ python s3_uploader.py
Init parquet packer...
Init kuksa VAL server client...
connect to wss://localhost:8090
Websocket connected securely.
Init S3 client...
Existing buckets:
  kuksa
receive loop started
update file kuksa_2022-Dec-07_09_02_55.691982.parquet to s3://kuksa/kuksa_2022-Dec-07_09_02_55.691982.parquet
^C
```

You should now see a file on your bucket:
```console
$ aws s3 ls s3://kuksa/ --endpoint-url http://localhost:8000
2022-12-07 09:03:05      14404 kuksa_2022-Dec-07_09_02_55.691982.parquet
```

## View generated parquet files

You can use the python command line tool [parquet-tools](https://pypi.org/project/parquet-tools/) to view generated parquet files:

```console
$ pip install parquet-tools
$ parquet-tools show s3://bucket-name/prefix/*
$ parquet-tools show kuksa.parquet
```

> **Note**
> `parquet-tools show s3://bucket-name/prefix/*` only works against actual Amazon S3 buckets, not locally hosted ones.
