# Kuksa VISS Client
![kuksa.val Logo](../doc/pictures/logo.png)

`kuksa.val` is a part of the opensource project [eclipse kuksa](https://www.eclipse.org/kuksa/).
More about `kuksa.val` can be found under the root [README.md](../README.md).
Here is a collection of clients, which can feed or handle data with the `kuksa.val` server.

## Python SDK
The python sdk is the easiest way for you to develop your own kuksa.val clients to interactive with the `kuksa.val` server.

### Installation
```
pip install kuksa-viss-client
```

### Usage

import the sdk
```
>>> import kuksa_viss_client
>>> kuksa_viss_client.__version__
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
>>> client = kuksa_viss_client.KuksaClientThread(config)
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
Using the python sdk, we implemented a command-line test client. When you have installed the kuksa-viss-client package via pip you can run the test client directly by executing

```bash
$ kuksa_viss_client
```

![Alt text](../doc/pictures/testclient_basic.gif "test client usage")


Refer help for further information
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

Using the testclient, it is also possible to update and extend the VSS data structure. More details can be found [here](../doc/liveUpdateVSSTree.md).
#### Docker
You can build a docker image of the testclient using the [`Dockerfile`](./Dockerfile). Not the most effcient way to pack a small python script, but it is easy to get started. The Dockerfile needs to be executed on the parent directory (so it include the needed certificates and `pip` package configuration).

To run the builded image:

```
docker run --rm -it --net=host <image-id-from docker-build>
```

`--rm` ensures we do not keep the docker continer lying aroind after closing the vss-testclient and `--net=host` makes sure you can reach locally running kuksa.val-server or kuksa-val docker with port forwarding on the host using the default `127.0.0.1` address.

