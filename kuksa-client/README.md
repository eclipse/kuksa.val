# KUKSA Python Client and SDK
![kuksa.val Logo](https://raw.githubusercontent.com/eclipse/kuksa.val/0.2.5/doc/pictures/logo.png)

KUKSA Python Client and SDK is a part of the open source project [Eclipse KUKSA](https://www.eclipse.org/kuksa/).
More about Eclipse KUKSA can be found in the [repository](https://github.com/eclipse/kuksa.val).

## Introduction

KUKSA Python Client and SDK provides both a command-line interface (CLI) and a standalone library to interact with either
[KUKSA Server](https://github.com/eclipse/kuksa.val/tree/master/kuksa-val-server) or
[KUKSA Databroker](https://github.com/eclipse/kuksa.val/tree/master/kuksa_databroker).


## Installing the client and starting its CLI

The fastest way to start using KUKSA Python Client is to install a pre-built version from pypi.org:

```console
pip install kuksa-client
```

If you want to install from sources instead see [Building and running a local version](#building-and-running-a-local-version).

After you have installed the kuksa-client package via pip you can run the test client CLI directly by executing:

```console
kuksa-client
```

With default CLI arguments, the client will try to connect to a local Databroker, e.g. a server supporting the `kuksa.val.v1` protocol without using TLS. This is equivalent to executing

```console
kuksa-client grpc://127.0.0.1:55555
```


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

Connecting to VSS server at 127.0.0.1 port 55555 using KUKSA GRPC protocol.
TLS will not be used.
INFO 2023-09-15 18:48:13,415 kuksa_client.grpc No Root CA present, it will not be posible to use a secure connection!
INFO 2023-09-15 18:48:13,415 kuksa_client.grpc.aio Establishing insecure channel
gRPC channel connected.
Test Client>
```

If you wish to connect to a VISS server e.g. `kuksa-val-server` (not using TLS), you should instead run:

```console
kuksa-client ws://127.0.0.1:8090
```

### TLS with databroker

KUKSA Client uses TLS to connect to databroker when the schema part of the server URI is `grpcs`, i.e. a valid command to connect to a TLS enabled local databroker is

```
kuksa-client grpcs://localhost:55555
```

By default the KUKSA example Root CA and Client keys are used, but client keys have no effect currently as mutual authentication is not supported by KUKSA Databroker or KUKSA Server.


This call with all parameters specified give same effect:

```
kuksa-client --certificate ../kuksa_certificates/Client.pem --keyfile ../kuksa_certificates/Client.key --cacertificate ./kuksa_certificates/CA.pem grpcs://localhost:55555
```

There is actually no reason to specify client key and certificate, as mutual authentication is not supported in KUKSA Databroker,
so the command can be simplified like this:

```
kuksa-client --cacertificate ./kuksa_certificates/CA.pem grpcs://localhost:55555
```

The example server protocol list 127.0.0.1 as an alternative name, but the TLS-client currently used does not accept it,
instead a valid server name must be given as argument.
Currently `Server` and `localhost` are valid names from the example certificates.

```
kuksa-client --cacertificate ../kuksa_certificates/CA.pem --tls-server-name Server grpcs://127.0.0.1:55555
```

### TLS with val-server
Val-server also supports TLS. KUKSA Client uses TLS to connect to val-server when the schema part of the server URI is `wss`.  A valid command to connect to a local TLS enabled val-server is

```
kuksa-client wss://localhost:8090
```

This corresponds to this call:

```
kuksa-client --cacertificate ../kuksa_certificates/CA.pem wss://localhost:8090
```

In some environments the `--tls-server-name` argument must be used to specify alternative server name
if connecting to the server by numerical IP address like `wss://127.0.0.1:8090`.

### Authorizing against KUKSA Server
If the connected KUKSA Server or KUKSA Databroker require authorization the first step after a connection is made is to authorize. KUKSA Server and KUKSA Databroker use different token formats.

The jwt tokens for testing can either be found under [../kuksa_certificates/jwt](../kuksa_certificates/jwt)
or you can also use following command inside `kuksa-client` to find the via `pip` installed certificate directory.

```console
Test Client> printTokenDir
```
Select one of the tokens and use the `authorize` command like below:

```console
Test Client> authorize /some/path/kuksa_certificates/jwt/super-admin.json.token
```

### Authorizing against KUKSA Databroker

If connecting to Databroker the command `printTokenDir` is not much help as it shows the default token directories
for KUKSA Server example tokens. If the KUKSA Databroker use default example tokens then one of the
tokens in [../jwt](../jwt) can be used, like in the example below:

```console
Test Client> authorize /some/path/jwt/provide-all.token
```

## Usage Instructions

Refer help for further information

```console
Test Client> help -v

Documented commands (use 'help -v' for verbose/'help <topic>' for details):

Communication Set-up Commands
================================================================================
authorize           Authorize the client to interact with the server
connect             Connect to a VSS server
disconnect          Disconnect from the VISS/gRPC Server
getServerAddress    Gets the IP Address for the VISS/gRPC Server

Info Commands
================================================================================
info                Show summary info of the client
printTokenDir       Show default token directory
version             Show version of the client

Kuksa Interaction Commands
================================================================================
getMetaData          Get MetaData of the path
getTargetValue       Get the value of a path
getTargetValues      Get the value of given paths
getValue             Get the value of a path
getValues            Get the value of given paths
setTargetValue       Set the target value of a path
setTargetValues      Set the target value of given paths
setValue             Set the value of a path
setValues            Set the value of given paths
subscribe            Subscribe the value of a path
subscribeMultiple    Subscribe to updates of given paths
unsubscribe          Unsubscribe an existing subscription
updateMetaData       Update MetaData of a given path
updateVSSTree        Update VSS Tree Entry

```

This is an example showing how some of the commands can be used:

![try kuksa-client out](https://raw.githubusercontent.com/eclipse/kuksa.val/master/doc/pictures/testclient_basic.gif "test client usage")

### Syntax for specifying data in the command line interface

Values used as argument to for example `setValue` shall match the type given. Quotes (single and double) are
generally not needed, except in a few special cases. A few valid examples on setting float is shown below:

```
setValue Vehicle.Speed 43
setValue Vehicle.Speed "45"
setValue Vehicle.Speed '45.2'
```

For strings escaped quotes are needed if you want quotes to be sent to Server/Databroker, like if you want to store
`Almost "red"` as value. Alternatively you can use outer single quotes and inner double quotes.

*NOTE: KUKSA Server and Databroker currently handle (escaped) quotes in strings differently!*
*The behavior described below is in general correct for KUKSA Databroker, but result may be different if interacting with KUKSA Server!*
*For consistent behavior it is recommended not to include (escaped) quotes in strings, except when needed to separate values*

The two examples below are equal:

```
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect 'Almost \"red\"'
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect 'Almost "red"'
```

Alternatively you can use inner single quotes, but then the value will be represented by double quotes (`Almost "blue"`)
when stored anyhow.

```
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect "Almost 'blue'"
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect "Almost \'blue\'"
```

If not using outer quotes the inner quotes will be lost, the examples below are equal.
Leading/trailing spaces are ignored.

```
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect Almost 'green'
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect Almost green
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect 'Almost green'
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect "Almost green"
setValue Vehicle.Cabin.Light.InteractiveLightBar.Effect 'Almost green         '
```

It is possible to set array values. In general the value should be a valid JSON representation of the array.
For maximum compatibility for both KUKSA Server and KUKSA Databroker the following recommendations applies:

* Always use single quotes around the array value. For some cases, like if there is no blanks or comma in the value, it is not needed, but it is good practice.
* Always use double quotes around string values.
* Never use single quotes inside string values
* Double quotes inside string values are allowed but must be escaped (`\"`)

Some examples supported by both KUKSA databroker and KUKSA Server are shown below

Setting a string array in KUKSA Databroker with simple identifiers is not a problem.
Also not if they contain blanks

```
// Array with two string elements
setValue Vehicle.OBD.DTCList '["abc","def"]'
// Array with two int elements (Note no quotes)
setValue Vehicle.SomeInt '[123,456]'
// Array with two elements, "hello there" and "def"
setValue Vehicle.OBD.DTCList '["hello there","def"]'
// Array with doubl quotes in string value; hello "there"
setValue Vehicle.OBD.DTCList '["hello, \"there\"","def"]'
```

### Updating VSS Structure

Using the test client, it is also possible to update and extend the VSS data structure.
More details can be found [here](https://github.com/eclipse/kuksa.val/blob/master/doc/KUKSA.val_server/liveUpdateVSSTree.md).

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

## Building and running a local version

For development purposes it may be necessary to customize the code for the client and run a locally built version.
First we suggest you create a dedicated [python virtual environment](https://docs.python.org/3/library/venv.html) for kuksa-client:

```console
mkdir --parents ~/.venv
python3 -m venv ~/.venv/kuksa-client
source ~/.venv/kuksa-client/bin/activate  # Run this every time you want to activate kuksa-client's virtual environment
```

Your prompt should change to somehting indicating you are in the virutal environment now, e.g.

```console
(kuksa-client) $
```
Inside the virtual environment install the dependencies
```console
pip install --upgrade pip
```

Now in order to ensure local `*.py` files will be used when running the client, we need to install kuksa-client in editable mode:

```console
pip install -r requirements.txt -e .
```

If you wish to also install test dependencies, run instead:
```console
pip install -r test-requirements.txt -e ".[test]"
```

If you ever wish to upgrade provided requirements, see [Requirements](docs/requirements.md).

Now you should be able to start using `kuksa-client`:
```console
kuksa-client --help
```

Whenever you want to exit kuksa-client's virtual environment, simply run:
```console
deactivate
```

## Using Docker
You can build a docker image of the testclient using the [`Dockerfile`](./Dockerfile).
Not the most effcient way to pack a small python script, but it is easy to get started.
The Dockerfile needs to be executed on the parent directory (so it include the needed certificates and `pip` package configuration).


```console
cd /some/dir/kuksa.val
docker build -f kuksa-client/Dockerfile -t kuksa-client:latest .
```

To run the newly built image:

```console
docker run --rm -it --net=host kuksa-client:latest --help
```

Notes:
- `--rm` ensures we do not keep the docker container lying around after closing kuksa-client and `--net=host` makes sure you can reach locally running kuksa.val-server or kuksa-val docker with port forwarding on the host using the default `127.0.0.1` address.
- CLI arguments that follow image name (e.g. `kuksa-client:latest`) will be passed through to kuksa-client entry point (e.g. `--help`).

## Running test suite & quality checks

This project uses pytest as its test framework and pylint as its linter.
To run the test suite:

```console
pytest
```

To run the linter:
```console
pylint kuksa_client
```

## Python library

`kuksa-client` also provides a library to allow you to develop your own application that interacts with either
`kuksa-val-server` or `kuksa_databroker`.


### Usage

Import library's main package.
```python
>>> import kuksa_client
>>> kuksa_client.__version__
'<your version, e.g. 0.1.7>'
```

This package holds different APIs depending on your application's requirements.
For more information, see ([Documentation](https://github.com/eclipse/kuksa.val/blob/master/kuksa-client/docs/main.md)).


### TLS configuration

Clients like [KUKSA CAN Feeder](https://github.com/eclipse/kuksa.val.feeders/tree/main/dbc2val)
that use KUKSA Client library must typically set the path to the root CA certificate.
If the path is set the VSSClient will try to establish a secure connection.

```
# Shall TLS be used (default False for Databroker, True for KUKSA Server)
# tls = False
tls = True

# TLS-related settings
# Path to root CA, needed if using TLS
root_ca_path=../../kuksa.val/kuksa_certificates/CA.pem
# Server name, typically only needed if accessing server by IP address like 127.0.0.1
# and typically only if connection to KUKSA Databroker
# If using KUKSA example certificates the names "Server" or "localhost" can be used.
# tls_server_name=Server
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

2. ``ImportError: cannot import name 'types_pb2' from 'kuksa.val.v1'``:
It sometimes happens that ``_pb2*.py`` files are not generated on editable installations of kuksa_client.
In order to manually generate those files and get more details if anything fails, run:
```console
python setup.py build_pb2
```
