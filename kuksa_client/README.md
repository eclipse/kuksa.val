# kuksa.val Clients
![kuksa.val Logo](../doc/pictures/logo.png)

`kuksa.val` is a part of the opensource project [eclipse kuksa](https://www.eclipse.org/kuksa/).
More about `kuksa.val` can be found under the parent [README.md](../README.md).
Here is a collection of clients, which can feed or handle data with the `kuksa.val` server.

## Python SDK
The python sdk is the easiest way for you to develop your own kuksa.val clients to interactive with the `kuksa.val` server.

### Requirements
- Python3

### Installation
```
pip install kuksa-client
```

### Usage
```
>>> import kuksa_client
>>> kuksa_client.__version__
'<your version, e.g. 0.1.7>'
>>>
>>> client = kuksa_client.KuksaClientThread(config)
>>> client.start()
```

## Test client
Using the python sdk, we implemented a command-line test client.

#### Usage
Set the Server IP Address & Connect
```
VSS Client> setServerIP 127.0.0.1 8090
VSS Client> connect
```

Refer help for further issues
```
VSS Client> help -v
```

## Other python clients 
Name | Description
---- | -----------
[dapr pubsub](./dapr/pubsub) | Data bridge from `kuksa.val` to dapr `pubsub` component
[dapr state_store](./dapr/state_store) | Store data to dapr `statestore` component
[s3](./s3) | Data bridge to s3 server
[GPS feeder](./feeder/gps2val) | GPS data source for `kuksa.val` server
[DBC feeder](./feeder/dbc2val) | DBC feeder for `kuksa.val` server

## Clients in other language 
Name | Description
[web client](./web-client) | A static webpage, which virsualize `kuksa.val` data
[node red](./node-red) | Examples of `kuksa.val` clients using node-red flows
