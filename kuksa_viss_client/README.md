# Kuksa VISS Client
![kuksa.val Logo](../doc/pictures/logo.png)

`kuksa.val` is a part of the opensource project [eclipse kuksa](https://www.eclipse.org/kuksa/).
More about `kuksa.val` can be found under the root [README.md](../README.md).

## Introduction

`kuksa_viss_client` is a command-line test client which can be used to interact with the [KUKSA val server](../kuksa-val-server/README.md)

## Installing and Starting the client

The fastest way to start using the kuksa-viss-client is to install a pre-built version with pip.
Instructions on how to build and run locally can be found further down in this document.

```
pip install kuksa-viss-client
```


After you have installed the kuksa-viss-client package via pip you can run the test client directly by executing

```bash
$ kuksa_viss_client
```
If everything works as expected and the server can be contacted you will get an output similar to below.


```bash
Welcome to kuksa viss client version <some_version>

                  `-:+o/shhhs+:`                  
                ./oo/+o/``.-:ohhs-                
              `/o+-  /o/  `..  :yho`              
              +o/    /o/  oho    ohy`             
             :o+     /o/`+hh.     sh+             
             +o:     /oo+o+`      /hy             
             +o:     /o+/oo-      +hs             
             .oo`    oho `oo-    .hh:             
              :oo.   oho  -+:   -hh/              
               .+o+-`oho     `:shy-               
                 ./o/ohy//+oyhho-                 
                    `-/+oo+/:.             

Default tokens directory: /some/path/kuksa_certificates/jwt

connectj to wss://127.0.0.1:8090
Websocket connected securely.
Test Client>
```

The next step is to authorize against the server.
The jwt tokens for testing can either be found under [kuksa_certificates/jwt](../kuksa_certificates/jwt) or you can also use following command inside `kuksa_viss_client` to find the via `pip` installed certificate directory.

```bash
Test Client> printTokenDir
```
Select one of the tokens and use the `authorize` command like below. 

```bash
Test Client> authorize /some/path/kuksa_certificates/jwt/super-admin.json.token
```

## Usage Instructions

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

Info Commands
================================================================================
info                Show summary info of the client
printTokenDir       Show token directory
version             Show version of the client

Kuksa Interaction Commands
================================================================================
getMetaData         Get MetaData of the path
getValue            Get the value of a path
setValue            Set the value of a path
updateMetaData      Update MetaData of a given path
updateVISSTree      Update VISS Tree Entry
```

This is an example showing how some of the commands can be used:

![try kuksa_viss_client out](../doc/pictures/testclient_basic.gif "test client usage")

### Updating VSS Structure

Using the testclient, it is also possible to update and extend the VSS data structure. More details can be found [here](../doc/liveUpdateVSSTree.md).

**Note**: You can also use `setValue` to change the value of an array, but the value should not contains any non-quoted spaces. Consider the following examples:

```
Test Client> setValue Vehicle.OBD.DTCList ["dtc1","dtc2"]
{
    "action": "set", 
    "requestId": "f7b199ce-4d86-4759-8d9a-d6f8f935722d", 
    "ts": "2022-03-22T17:19:34.1647965974Z"
}

Test Client> setValue Vehicle.OBD.DTCList '["dtc1", "dtc2"]'
{
    "action": "set", 
    "requestId": "d4a19322-67d8-4fad-aa8a-2336404414be", 
    "ts": "2022-03-22T17:19:44.1647965984Z"
}

Test Client> setValue Vehicle.OBD.DTCList ["dtc1", "dtc2"]
usage: setValue [-h] Path Value
setValue: error: unrecognized arguments: dtc2 ]
```


## Building and Running a local version

For development purposes it may be necessary to customize the code for the client and run a locally built version.
The commands below can be used for that purpose. this will assure that local `*.py` files will be used when running the client.

```bash
cd kuksa_viss_client
pipenv shell
pipenv lock
pipenv sync
python __main.py__
```

After testing you can use `quit` to exit the client and `exit`to exit the pipenv shell. In subsequent runs some parts can be skipped:

```bash
cd kuksa_viss_client
pipenv shell
python __main.py__
```

## Using Docker
You can build a docker image of the testclient using the [`Dockerfile`](./Dockerfile). Not the most effcient way to pack a small python script, but it is easy to get started. The Dockerfile needs to be executed on the parent directory (so it include the needed certificates and `pip` package configuration). 


```bash
cd /some/dir/kuksa.val
docker build -f kuksa_viss_client/Dockerfile -t kuksa-client:latest .
```

To run the builded image:

```
docker run --rm -it --net=host kuksa-client:latest
```

`--rm` ensures we do not keep the docker continer lying aroind after closing the vss-testclient and `--net=host` makes sure you can reach locally running kuksa.val-server or kuksa-val docker with port forwarding on the host using the default `127.0.0.1` address.


## Python SDK
If you would like to develop your own `kuksa.val` client,
you can use the python sdk to interact with the `kuksa.val` server in a very easy way.


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

You have the following methods to interact with the `kuksa.val` server:

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

