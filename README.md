# KUKSA.VAL
![kuksa.val Logo](./doc/pictures/logo.png)

This is KUKSA.val, the KUKSA **V**ehicle **A**bstration **L**ayer.

[![Gitter](https://badges.gitter.im/kuksa-val.svg)](https://gitter.im/kuksa-val)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue.svg)](https://opensource.org/licenses/EPL-2.0)
[![Build Status](https://ci.eclipse.org/kuksa/buildStatus/icon?job=kuksa.val%2Fmaster)](https://ci.eclipse.org/kuksa/job/kuksa.val/job/master/)


KUKSA.val provides a [Genivi VSS data model](https://github.com/GENIVI/vehicle_signal_specification) describing data in a vehicle. This data is provided to applications using a variant based on the W3C VISS Interface. KUKSA.val supports VISS V1 https://www.w3.org/TR/vehicle-information-service/ and extensions as well as parts of the upcomming VISS2 standard (https://raw.githack.com/w3c/automotive/gh-pages/spec/Gen2_Core.html, https://raw.githack.com/w3c/automotive/gh-pages/spec/Gen2_Transport.html), that are applicable to in-vehicle VSS servers.

See [Supported Protocol](doc/protocol.md) for a detailled overview.

## Features
 - Websocket interface, TLS-secured or plain
 - [Experimental REST interface](doc/rest-api.md), TLS-secured or plain
 - [Fine-grained authorisation](doc/jwt.md) based on JSON Webtokens (RFC 7519)
 - Optional [JSON signing](doc/json-signing.md) of messages
 - Built-in MQTT publisher
 - [Python viss client](./kuksa_viss_client) to interactively explore and modify the VISS data points and data structure
 - Multiple [example apps](./kuksa_apps) in different programming languages to communicate with different frameworks 
 - Multiple [feeders](./kuksa_feeders) to provide vehicle data for the `kuksa.val` server


## Quick start

### Using  Docker Image
If you prefer to build kuksa.val yourself skip to [Building KUKSA.val](#Building-kuksaval).

Download a current docker image from our CI server:

https://ci.eclipse.org/kuksa/job/kuksa.val/job/master/

The container images should work with any OCI compliant container runtime, in this document we assume you are using docker

Import the docker image

```
docker load -i kuksa-val-b3084b9-amd64.tar.xz
```

Your build tag may vary, and for ARM machines you need to choose an arm64 images.

Prepare an empty directory `$HOME/kuksaval.config`.  Run the docker image using the tag reported by `docker load`:

```bash
docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL amd64/kuksa-val:b3084b9
```

More information on using the docker images can be found [here](doc/run-docker.md).

To learn, how to build your own docker image see [doc/build-docker.md](doc/build-docker.md).

If this is succesful you can skip to [using KUKSA.val](#Using-kuksaval).

## Building KUKSA.val
KUKSA.val uses the cmake build system. First install the required packages. On Ubuntu 20.04 this can be achieved by

```
sudo apt install cmake build-essential libboost1.67-all-dev libssl-dev libglib2.0-dev libmosquitto-dev 
```

When fetching the source, make sure you also get the needed submodules, e.g. by using the `--recursive` flag

```
git clone --recursive https://github.com/eclipse/kuksa.val.git
```

Then create a build folder inside the kuksa.val folder and execute cmake

```
mkdir build
cd build
cmake ..
```
If there are any missing dependencies, cmake will tell you. If everythig works fine, execute make

```
make
```

(if you have more cores, you can speed up compilation with something like  `make -j8`

Additional information about our cmake setup (in case you need adavanced options or intend to extend it) can be [found here](doc/cmake.md).



### Running kuksa.val
After you successfully built the kuksa.val server you can run it like this

```bash
#assuming you are inside kuksa.val/build directory
cd src
./kuksa-val-server  --vss ./vss_rel_2.0.json  --log-level ALL

```
Setting log level to `ALL` gives you some more information about what is going on.

You can also edit [config.ini](./config.ini) file to configure kuksa val server. This file will be copied to the build directory and used als default config,
if no other config file is specified using the command line option `-c/--config-file`.

For more information check [usage](doc/usage.md).



## Other topics

 * Experimental [D-Bus](doc/dbus.md) connector
