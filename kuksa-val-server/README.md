# KUKSA.VAL
![kuksa.val Logo](../doc/pictures/logo.png)

kuksa-val-server is a feature rich in-vehicle data server written in C++ providing authorized access to vehicle data.


Check [System Architecture](../doc/system-architecture.md) for an overview how KUKSA.val can be used and deployed in a modern Software Defined Vehicle.

kuksa-val-server serves signals described using the [COVESA VSS data model](https://github.com/COVESA/vehicle_signal_specification). VSS data is provided to applications using a variant based on the W3C VISS Interface or GRPC. KUKSA.val supports VISS V1 https://www.w3.org/TR/vehicle-information-service/ and extensions as well as parts of the upcomming VISS2 standard ([Gen2 Core](https://raw.githack.com/w3c/automotive/gh-pages/spec/VISSv2_Core.html), [Gen2 Transport](https://raw.githack.com/w3c/automotive/gh-pages/spec/VISSv2_Transport.html)), that are applicable to in-vehicle VSS servers.

See [Supported Protocol](../doc/protocol/README.md) for a detailled overview of the supported VISS feature.

## Features
 - Websocket interface, TLS-secured or plain
 - [Fine-grained authorisation](../doc/KUKSA.val_server/jwt.md) based on JSON Webtokens (RFC 7519)
 - Built-in MQTT publisher
 - Python [Kuksa Client](../kuksa-client) to interactively explore and modify the VSS data points and data structure
 - Multiple [example apps](../kuksa_apps) in different programming languages to communicate with different frameworks
 - Multiple [feeders](https://github.com/eclipse/kuksa.val.feeders/tree/main) to provide vehicle data for the `kuksa.val` server
 - Support most of data types, which is specified in [COVESA VSS data model](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/data_types/).


## Quick start

### Using Docker Image
If you prefer to build kuksa.val yourself skip to [Building KUKSA.val](#building-kuksaval).

Download a current KUKSA.val server docker image from one of our container registry:

- https://github.com/eclipse/kuksa.val/pkgs/container/kuksa.val%2Fkuksa-val

The container images should work with any OCI compliant container runtime, in this document we assume you are using docker

Pull the docker image

```
docker pull ghcr.io/eclipse/kuksa.val/kuksa-val:latest
```


Prepare an empty directory `$HOME/kuksaval.config`.  Run the desired docker image using `docker run`:

```bash
docker run -it --rm --net=host -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/kuksa-val:master
```

More information on using the docker images can be found [here](../doc/KUKSA.val_server/run-docker.md).

To learn, how to build your own docker image see [doc/build-docker.md](../doc/KUKSA.val_server/build-docker.md).

If this is successful you can skip to [using kuksa.val](#using-kuksaval).

## Building kuksa.val

First you need to fetch the source. Make sure you also get the needed submodules, e.g. by using the `--recursive` flag

```
git clone --recursive https://github.com/eclipse/kuksa.val.git
```

### Manually install dependencies
First install the required packages. On Ubuntu 20.04 this can be achieved by

```
sudo apt install cmake build-essential libssl-dev libmosquitto-dev
```


### Compiling
Create a build folder inside the kuksa-val-server folder and execute cmake

```bash
cd kuksa-val-server
mkdir build
cd build
cmake ..
```
If there are any missing dependencies, cmake will tell you. If everything works fine, execute make

```
make
```

(if you have more cores, you can speed up compilation with something like  `make -j8`

Additional information about our cmake setup (in case you need advanced options or intend to extend it) can be [found here](../doc/KUKSA.val_server/cmake.md).

### Running kuksa.val
After you successfully built the kuksa.val server you can run it like this

```bash
#assuming you are inside kuksa-val-server/build directory
cd src
./kuksa-val-server  --vss ./vss_release_4.0.json --log-level ALL

```
Setting log level to `ALL` gives you some more information about what is going on.

You can also edit [config.ini](./config.ini) file to configure kuksa val server. This file will be copied to the build directory and used als default config,
if no other config file is specified using the command line option `-c/--config-file`.

For more information check [usage](../doc/KUKSA.val_server/usage.md).

## Using kuksa.val
The easiest way to try `kuksa-val-server` out, is to use the test client [`kuksa-client`](../kuksa-client):

```
pip install kuksa-client
kuksa-client
```

![try kuksa-client out](../doc/pictures/testclient_basic.gif "test client usage")

The jwt tokens for testing can also be found under [kuksa_certificates/jwt](../kuksa_certificates/jwt).

You can also use the provided python sdk to develop your own `kuksa.val` clients. More details about `kuksa-client` can be found [here](../kuksa-client).

Additionally, you can use the [example apps](../kuksa_apps) and [feeders](https://github.com/eclipse/kuksa.val.feeders/tree/main) to handle vehicle data, interacting with `kuksa-val-server`.
