# KUKSA.VAL

This is KUKSA.val, the KUKSA **V**ehicle **A**bstration **L**ayer. 

Basically KUKSA.val provides a [Genivi VSS data model](https://github.com/GENIVI/vehicle_signal_specification) describing data in a vehicle. This data is provided to applications using various interfaces, i.e. a websocket interface based on
the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)




## Features
 - Websocket interface, TLS-secured or plain
 - [Experimental REST interface](doc/rest-api.md), TLS-secured or plain
 - [Fine-grained authorisation](doc/jwt.md) based on JSON Webtokens (RFC 7519)
 - Optional [JSON signing](doc/json-signing.md) of messages
 - Built-in MQTT publisher 

## Quick start

### Using  Docker Image
If you prefer to build kuksa.val yourself skip to [Building KUKSA.val](#Building-kuksaval)
Download current docker image from our CI

https://ci.eclipse.org/kuksa/job/kuksa.val/job/master/

Import docker image

```
sudo docker load -i kuksa-val-amd64.tar.bz2
```

Start docker image prepare an empty directore `$HOME/kuksaval.config` and using the tag repoted by `docker load`, run the container

```bash
sudo docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL amd64/kuksa-val:0.1.1
```

More information on using the docker images can be found [here](doc/run-docker.md).
To learn, how to build your own docker image see [doc/build-docker.md](doc/build-docker.md).

If this is succesful you can skip to [using KUKSA.val](#Using-kuksaval)

## Building KUKSA.val
KUKSA.val uses the cmake build system. First install the required packages. On Ubuntu 20.04 this can be achieved by

```
sudo apt install cmake  libboost1.67-all-dev   libssl-dev libglib2.0-dev
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

Additional information about our cmake setup (in case you need adavanced options or intend to extend it) can be [found here](doc/cmake.md)



### Running kuksa.val
After you successfully built the kuksa.val server you can run it like this

```bash
#assuming you are inside kuksa.val/build directory
cd src
./kuksa-val-server  --vss ./vss_rel_2.0.json  --log-level ALL

```
Setting log level to `ALL` gives you some more information about what is going on.

For more information check [usage](doc/usage.md).


## Using KUKSA.val

You can use the [testclient](./vss-testclient/) to test basic functions like _getMetaData_, _getValue_ and _setValue_ requests.

After starting the kuksa server using `./kuksa-val-server` command. You can start the testclient as follows:

![Alt text](./doc/pictures/testclient_basic.gif "test client usage")

Using the testclient, it is also possible to update and extend the VSS data structure. More details can be found [here](./doc/liveUpdateVSSTree.md).



## Other topics

 * Experimental [D-Bus](doc/dbus.md) connector



## D-BUS Backend Connection

The server also has d-bus connection, which could be used to feed the server with data from various feeders.
The W3C Sever exposes the below methods and these methods could be used (as methodcall) to fill the server with data.
  ```
  <interface name='org.eclipse.kuksa.w3cbackend'>
     <method name='pushUnsignedInt'>
       <arg type='s' name='path' direction='in'/>
       <arg type='t' name='value' direction='in'/>
     </method>
     <method name='pushInt'>
       <arg type='s' name='path' direction='in'/>
       <arg type='x' name='value' direction='in'/>
     </method>
     <method name='pushDouble'>
       <arg type='s' name='path' direction='in'/>
       <arg type='d' name='value' direction='in'/>
     </method>
     <method name='pushBool'>
       <arg type='s' name='path' direction='in'/>
       <arg type='b' name='value' direction='in'/>
     </method>
     <method name='pushString'>
       <arg type='s' name='path' direction='in'/>
       <arg type='s' name='value' direction='in'/>
     </method>
   </interface>
   ```