# Kuksa VISS Clients
![kuksa.val Logo](../doc/pictures/logo.png)

`kuksa.val` is a part of the opensource project [eclipse kuksa](https://www.eclipse.org/kuksa/).
More about `kuksa.val` can be found under the root [README.md](../README.md).
Here is a collection of clients, which can feed or handle data with the `kuksa.val` server.

## Python SDK
The python sdk is the easiest way for you to develop your own kuksa.val clients to interactive with the `kuksa.val` server.

### Installation
```
pip install kuksa-client
```

### Usage

import the sdk
```
>>> import kuksa_client
>>> kuksa_client.__version__
'<your version, e.g. 0.1.7>'
```

Setup a thread to connect with the `kuksa.val` server.
The following properties for the connection can be configured:
- `ip` default: "127.0.0.1"
- `port` default: 8090
- `insecure` default: `False`
- `cacertificate` default: "../kuksa_certificates/CA.pem"
- `certificate` default: "../kuksa_certificates/Client.pem"
- `key` default: "../kuksa_certificates/Client.key"

```
>>> config = {} 
>>> client = kuksa_client.KuksaClientThread(config)
>>> 
>>> # Start the client thread to connect with configured server
>>> client.start()
>>>
>>> # Close the connection and stop the client thread
>>> client.stop()
```

You have the following methods to interactive with the `kuksa.val` server:

```
# Do authorization by passing a jwt token or a token file
def authorize(self, token, timeout = 2)

# Update VISS Tree Entry 
def updateVISSTree(self, jsonStr, timeout = 5)

# Update Meta Data of a given path
def updateMetaData(self, path, jsonStr, timeout = 5)

# Get Meta Data of a given path
def getMetaData(self, path, timeout = 1)

# Set value to a given path
def setValue(self, path, value, timeout = 1)

# Get value to a given path
def getValue(self, path, timeout = 5)

# Subscribe value changes of to a given path.
# The given callback function will be called then, if the given path is updated:
#   updateMessage = await webSocket.recv()
#   callback(updateMessage)
def subscribe(self, path, callback, timeout = 5)
```

## Test client
Using the python sdk, we implemented a command-line test client.

#### Usage
After install the package from  `pip`. You can just start the test client with `kuksa_client`.

Refer help for further issues
```
VSS Client> help -v

Documented commands (use 'help -v' for verbose/'help <topic>' for details):

Communication Set-up Commands
================================================================================
authorize           Authorize the client to interact with the server
connect             
disconnect          Disconnect from the VSS Server
getServerAddress    Gets the IP Address for the VSS Server
setServerAddress    Sets the IP Address for the VSS Server

Kuksa Interaction Commands
================================================================================
getMetaData         Get MetaData of the path
getValue            Get the value of a path
setValue            Set the value of a path
updateMetaData      Update MetaData of a given path
updateVISSTree      Update VISS Tree Entry
```

## s3 uploader
This example client can bridge data from `kuksa.val` to a s3 server. After install the package and install the necessary packages:

```
pip install kuksa-client
pip install -r s3/requirements.txt
```

You can start the client by run:

```
kuksa_s3_client
```

More details can be found under the folder [s3](./s3).

## Other python clients 
Name | Description
---- | -----------
[dapr pubsub](./dapr/pubsub) | Data bridge from `kuksa.val` to dapr `pubsub` component
[dapr state_store](./dapr/state_store) | Store data to dapr `statestore` component
[GPS feeder](./feeder/gps2val) | GPS data source for `kuksa.val` server
[DBC feeder](./feeder/dbc2val) | DBC feeder for `kuksa.val` server

## Clients in other language 
Name | Description
---- | -----------
[web client](./web-client) | A static webpage, which virsualize `kuksa.val` data
[node red](./node-red) | Examples of `kuksa.val` clients using node-red flows
