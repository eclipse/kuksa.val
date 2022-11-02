# Kuksa Client
![kuksa.val Logo](https://raw.githubusercontent.com/eclipse/kuksa.val/0.2.5/doc/pictures/logo.png)

`kuksa.val` is a part of the opensource project [eclipse kuksa](https://www.eclipse.org/kuksa/).
More about `kuksa.val` can be found in the [repository](https://github.com/eclipse/kuksa.val).

## Introduction

`kuksa-client` is a command-line test client which can be used to interact with the [KUKSA val server](https://github.com/eclipse/kuksa.val/tree/0.2.5/kuksa-val-server)

## Installing and Starting the client

The fastest way to start using the kuksa-client is to install a pre-built version with pip.
Instructions on how to build and run locally can be found further down in this document.

```console
$ pip install kuksa-client
```


After you have installed the kuksa-client package via pip you can run the test client directly by executing

```console
$ kuksa-client
```

With default CLI arguments, the client will try to connect to a local VISS server e.g. `kuksa-val-server`.
If you wish to connect to a gRPC server e.g. `kuksa-databroker`, you should instead run:

```console
$ kuksa-client --ip 127.0.0.1 --port 55555 --protocol grpc --insecure
```
Note: `--insecure` is required because `kuksa-databroker` does not yet support encryption or authentication.

If everything works as expected and the server can be contacted you will get an output similar to below.


```console
Welcome to Kuksa Client version <some_version>

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
The jwt tokens for testing can either be found under [kuksa_certificates/jwt](https://github.com/eclipse/kuksa.val/tree/0.2.5/kuksa_certificates/jwt) or you can also use following command inside `kuksa-client` to find the via `pip` installed certificate directory.

```console
Test Client> printTokenDir
```
Select one of the tokens and use the `authorize` command like below.

```console
Test Client> authorize /some/path/kuksa_certificates/jwt/super-admin.json.token
```

## Usage Instructions

Refer help for further information
```console
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
updateVSSTree      Update VSS Tree Entry
```

This is an example showing how some of the commands can be used:

![try kuksa-client out](https://raw.githubusercontent.com/eclipse/kuksa.val/0.2.5/doc/pictures/testclient_basic.gif "test client usage")

### Updating VSS Structure

Using the testclient, it is also possible to update and extend the VSS data structure. More details can be found [here](https://github.com/eclipse/kuksa.val/blob/0.2.5/doc/liveUpdateVSSTree.md).

**Note**: You can also use `setValue` to change the value of an array, but the value should not contains any non-quoted spaces. Consider the following examples:

```console
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
First we suggest you create a dedicated [python virtual environment](https://docs.python.org/3/library/venv.html) for kuksa-client:

```console
$ mkdir --parents ~/.venv
$ python3 -m venv ~/.venv/kuksa-client
$ source ~/.venv/kuksa-client/bin/activate  # Run this every time you want to activate kuksa-client's virtual environment
(kuksa-client) $ pip install --upgrade pip
```

Now in order to ensure local `*.py` files will be used when running the client, we need to install kuksa-client in editable mode:
```console
(kuksa-client) $ pip install --editable .
```

If you wish to also install test dependencies, run instead:
```console
(kuksa-client) $ pip install --editable ".[test]"
```

Now you should be able to start using `kuksa-client`:
```console
(kuksa-client) $ kuksa-client --help
```

Whenever you want to exit kuksa-client's virtual environment, simply run:
```console
(kuksa-client) $ deactivate
$
```

## Using Docker
You can build a docker image of the testclient using the [`Dockerfile`](./Dockerfile). Not the most effcient way to pack a small python script, but it is easy to get started. The Dockerfile needs to be executed on the parent directory (so it include the needed certificates and `pip` package configuration).


```console
$ cd /some/dir/kuksa.val
$ docker build -f kuksa-client/Dockerfile -t kuksa-client:latest .
```

To run the newly built image:

```console
$ docker run --rm -it --net=host kuksa-client:latest --help
```

Notes:
- `--rm` ensures we do not keep the docker container lying around after closing kuksa-client and `--net=host` makes sure you can reach locally running kuksa.val-server or kuksa-val docker with port forwarding on the host using the default `127.0.0.1` address.
- CLI arguments that follow image name (e.g. `kuksa-client:latest`) will be passed through to kuksa-client entry point (e.g. `--help`).

## Running test suite & quality checks

This project uses pytest as its test framework and pylint as its linter.
To run the test suite:

```console
$ pytest
```

To run the linter:
```console
$ pylint kuksa_client
```

## Python SDK

If you would like to develop your own `kuksa.val` client,
you can use the python sdk to interact with the `kuksa.val` server in a very easy way.


### Usage

Import the sdk
```python
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

```python
>>> config = {}
>>> client = kuksa_client.KuksaClientThread(config)
>>>
>>> # Start the client thread to connect with configured server
>>> client.start()
>>>
>>> # Close the connection and stop the client thread
>>> client.stop()
```

You have the following methods to interact with `kuksa-val-server` or `kuksa-databroker`:

```python
# Do authorization by passing a jwt token or a token file
def authorize(self, token, timeout=2):
    ...

# Update VSS Tree Entry
def updateVSSTree(self, jsonStr, timeout=5):
    ...

# Update Meta Data of a given path
def updateMetaData(self, path, jsonStr, timeout=5):
    ...

# Get Meta Data of a given path
def getMetaData(self, path, timeout=1):
    ...

# Set value to a given path
def setValue(self, path, value, timeout=1):
    ...

# Get value to a given path
def getValue(self, path, timeout=5):
    ...

# Subscribe value changes of to a given path.
# The given callback function will be called then, if the given path is updated.
def subscribe(self, path, callback, timeout=5):
    ...
```

## Troubleshooting

1. The server/data broker is listening on its port but my client is unable to connect to it and returns an error:
```console
Error: Websocket could not be connected or the gRPC channel could not be created.
```
If you're running both client and server on your local host, make sure that:
- `localhost` domain name resolution is configured properly on your host.
- You are not using any proxies for localhost e.g. setting the `no_proxy` environment variable to `localhost,127.0.0.1`.
- If you are using the `gRPC` protocol in secure mode, the server certificate should have `CN = localhost` in its subject.
