# KUKSA.VAL

This is the KUKSA vehicle abstration layer. It is based on the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)

This implementation can also provide additional functionality not (yet) available in (draft) standard documents.

## Features
 - Multi-client server implementing Web-Socket platform communication, with support for both secure [SSL] and insecure [plain] connections. Feature status:
 - User authorization based on industry standard RFC 7519 as JSON Web Tokens
 - Optional JSON signing of messages, described in **_JSON signing_** chapter
 - Multi-client server implementing experimental REST API based on standard specification. REST API specification is available as OpenAPI 3.0 definition available in [doc/rest-api.yaml](doc/rest-api.yaml) file.

 Specific list of of features is listed in table below:

  | Feature       | Status        |
  | ------------- | ------------- |
  | GET/SET       | :heavy_check_mark:|
  | PUB/SUB  | :heavy_check_mark: |
  | GETMETA  | :heavy_check_mark: |
  | Secure Web-Socket  | :heavy_check_mark: |
  | Authentification  | :heavy_check_mark: |
  | JSON signing    | :heavy_check_mark:  |
  | REST API | :heavy_check_mark:  |
  | MQTT Publisher | :construction:  |


## Building KUKSA.VAL
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

(if you have more cores, you can speed up compilation with something like  `make -j 8`

Additional information about our cmake setup (in case you need adavanced options or intend to extend it) can be [found here](doc/cmake.md)




# Running kuksa.val
After you successfully built the kuksa.val server you can run it like this

```bash
#assuming you are inside kuksa.val/build directory
cd src
./w3c-visserver  --vss ./vss_rel_2.0.json  --log-level ALL

```
Setting log level to `ALL` gives you some more information about what is going on.


## KUKSA.VAL Parameters
Below are presented W3C-Server parameters available for user control:
- **--help** - Show W3C-Server usage and exit
- **--vss** [mandatory] - Path to VSS data file describing VSS data tree structure which W3C-Server shall handle. Sample 'vss_rel_2.0.json' file can be found [here](./unit-test/vss_rel_2.0.json).
- **--config-file** [optional] - Path to configuration file with W3C-Server input parameters.    
  Configuration file can replace command-line parameters and through different files multiple configurations can be handled more easily (e.g. test and production setup).
  Sample of configuration file parameters is shown below:
  ```
  vss=vss_rel_2.0.json
  cert-path=.
  insecure=
  log-level=ALL
  ```
- **--cert-path** [mandatory] - Directory path where 'Server.pem', 'Server.key' and 'jwt.key.pub' are located. Server demo certificates are located in [../examples/demo-certificates](../examples/demo-certificates) directory of git repo. Certificates from 'demo-certificates' are automatically copied to build directory, so invoking '_--cert-path=._' should be enough when demo certificates are used.  
If user needs to use or generate their own certificates, see chapter [_Certificates_](#Certificates) for more details.  
For authorizing client, file 'jwt.key.pub' contains public key used to verify that JWT authorization token is valid. To generated different 'jwt.key.pub' file, see chapter [_Permissions_](#Permissions) for more details.
- **--insecure** [optional] - By default, W3C-Server shall accept only SSL (TLS) secured connections. If provided, W3C-Server shall also accept plain un-secured connections for Web-Socket and REST API connections, and also shall not fail connections due to self-signed certificates.
- **--address** [optional] - If provided, W3C-Server shall use different server address than default _'localhost'_.
- **--port** [optional] - If provided, W3C-Server shall use different server port than default '8090' value.
- **--log-level** [optional] - Enable selected log level value. To allow for different log level combinations, parameter can be provided multiple times with different log level values.



Depending on build options and provided parameters, W3C-Server will provide different features.
Chapter [_Parameters_](#Parameters) shall describe different mandatory and optional parameters in more detail.

Default configuration shall provide both Web-Socket and REST API connectivity.

## JWT Permissions
The kuksa.val server uses JSON Web Tokens (JWT) to authorize clients. For testing you can use the [Super Admin Token](certificates/jwt/super-admin.json.token). To learn how to to set up tokens go to [kuksa.val JWT documentation](doc/jwt.md).

## Web-Socket specific testing

This covers only the basic functions like _get_, _set_ and _getmetadata_ requests. You could skip this and take a look at the unit-test to get better idea about the implementation.

You could also checkout the in-vehicle apps in the [kuksa.apps](https://github.com/eclipse/kuksa.apps) repo which work with the server.

Now the apps are ready for testing. Run w3c-visserver using `./w3c-visserver` command and then in a separate terminal start testclient using `./testclient`.

Testclient should connect to the w3c-visserver and promt a message as below
![Alt text](./doc/pictures/test1.png?raw=true "test1")

Authenticate with the server using the JWT token
![Alt text](./doc/pictures/test4.png?raw=true "test4")

Enter the vss path and function as set and a dummy integer value.
![Alt text](./doc/pictures/test2.png?raw=true "test2")

Enter the same vss path as above and fuction as get. You should receive the previously set value in the JSON response.
![Alt text](./doc/pictures/test3.png?raw=true "test3")

## REST API specific testing

There is number of options to exercise REST API.

_**NOTE:** If using SSL connections and self-signed certificates, make sure that 'CA.pem' or corresponding file to generated certificates is imported into browser (or other tool) used for testing. Also user can try to disable certificate verification.  
Reason for this is that browsers automatically try to verify validity of server certificates, so secured connection shall fail with default configuration._

Similar to above mentioned testclient, there is available [client test page](./test/web-client/index.html) in git repo to aid testing.
Test page support custom GET, PUT and POST HTTP requests to W3C-Server. Additional benefit is that it can automatically generate JWT token based on input token value and provided Client key which is used in authorization. Note that if users changes Client key, user must also update 'jwt.key.pub' with corresponding public key.

Additional tool which is quite useful is [Swagger](https://editor.swagger.io). It is a dual-use tool which allows for writing OpenAPI specifications, but also generates runnable REST API samples for moslient test endpoints.
Open Swagger editor and import our REST API [definition](./doc/rest-api.yaml) file. Swagger shall generate HTML page with API documentation. When one of the endpoints is selected, 'try' button appears which allows for making REST requests directly to running W3C-Server.

# How-to's

### JSON Signing

JSON Signing has been introduced additionally to sign the JSON response for GET and SUBSCRIBE Response. By default this has been disabled. To enable this feature go to visconf.hpp file and uncomment the line `define JSON_SIGNING_ON`. Please note, JSON signing works only with a valid pair of public / private certificate. For testing, you could create example certificates by following the below steps.
Do not add any passphrase when asked for.

```
ssh-keygen -t rsa -b 4096 -m PEM -f signing.private.key
openssl rsa -in signing.private.key  -pubout -outform PEM -out signing.public.key
```

Copy the files signing.private.key & signing.public.key to the build directory.

The client also needs to validate the signed JSON using the public certificate when JSON signing is enabled in server.

This could also be easily extended to support JSON signing for the requests as well with very little effort.

## Certificates

Go to examples/demo-certificates folder. Make changes in the openssl.cnf file regarding the Company name and the allowed IPs and DNS server names. Make sure you also add the IPs and DNS to v3.ext file as well.

1. openssl genrsa -out MyRootCA.key 2048
2. openssl req -x509 -new -nodes -key MyRootCA.key -sha256 -days 1024 -out MyRootCA.pem -config openssl.cnf
3. openssl genrsa -out MyClient1.key 2048
4. openssl req -new -key MyClient1.key -out MyClient1.csr -config openssl.cnf
5. openssl x509 -req -in MyClient1.csr -extfile v3.ext -CA MyRootCA.pem -CAkey MyRootCA.key -CAcreateserial -out MyClient1.pem -days 1024 -sha256
6. Rename the MyClient1.pem and MyClient1.key to Server.pem and Server.key
7. follow step 3 to 5 to create another set of certs.
8. Rename the MyClient1.pem and MyClient1.key to Client.pem and Client.key and rename MyRootCA.pem and MyRootCA.key to CA.pem and CA.key respectively.

Steps were taken from [here]( https://kb.op5.com/pages/viewpage.action?pageId=19073746#sthash.GHsaFkZe.WDGgcOja.dpbs) & [here](https://stackoverflow.com/questions/18233835/creating-an-x509-v3-user-certificate-by-signing-csr).



## Coverage

Coverage information will be generated automatically for W3C-Server core library sources When _CMAKE_BUILD_TYPE_ is set to *Coverage* and GCC or Clang are used as compilers.

While coverage information will be generated, generation of reports are left to be handled by the user and its tool preferences.

Example of way for generating reports for different compilers is shown below.
We will use unit tests to generate coverage information.
Setting up build correctly, depending on compiler

### GCC compiler

For GCC compiler, as an example, we can use [_gcovr_](https://gcovr.com/en/stable/) tool to generate reports for generated coverage information.

```
# Make or goto out-of-source build Directory

# Configure build
CXX=g++ CC=gcc  cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

# make everything needed
make -j

# goto and run unit-tests
cd unit-test
./w3c-unit-test

# goto build root
cd ..

# generate coverage information with gcovr
gcovr -v --html -r ../src/ src/  -o coverage.html
```

After executing, _coverage.html_ file will be generated with detailed coverage information for core sources.

### Clang compiler

For Clang compiler, as an example, we can use [llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) tool to generate reports for generated coverage information.

```
# Make or goto out-of-source build Directory

# Configure build
CXX=clang++ CC=clang  cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_UNIT_TEST=ON ..

# make everything needed
make -j

# goto and run unit-tests
cd unit-test
./w3c-unit-test

# convert raw coverage data to indexed
llvm-profdata merge -sparse default.profraw -o default.profdata

# generate coverage information with llvm-cov
llvm-cov show  --format=html ../src/libw3c-visserver-core.so -instr-profile=default.profdata > coverage.html
```
After executing, _coverage.html_ file will be generated with detailed coverage information for core sources.

## Docker environment

To ease setup and increase development time, [Docker](https://www.docker.com/resources/what-container) can be used for any stage in develop or run process of the W3C-Server.

Docker [docker/Dockerfile](docker/Dockerfile) image specification is defined with complete infrastructure to develop and run W3C-Server. Any user can get Docker, build image and run, without any local machine configuration time wasted.

User can use existing git repo or clone one inside of existing Docker image instance.


### Build Docker image

You can build a docker image for KUKSA.val. You can cross compile an image for other platofrms such as `arm64`. To do this, make sure you have installed docker and qemu-user-static

```
sudo apt install qemu-user-static
```

Enable dockers experimental features by creating a `daemon.json`  file in `/etc/docker/` and add

```json
{
  "experimental" : true
}
```
After creeating the configuration you need to restart docker

```
systemctl restart docker
```

To build a docker for an x86/amd64 platform from `kuksa.val` directory do

```
cd docker
sudo  ./build.sh amd64
```

To build an arm64 call the build script like this 

```
sudo  ./build.sh arm64
```

Beware, that this needs a fast computer as compilation runs through qemu (emulating the target processor in software).


In case that network proxy configuration is needed, make sure to export _http_proxy_ and _https_proxy_ environment variables with correct proxy configuration, as shown below:
```
sudo docker build . --build-arg  http_proxy=$http_proxy --build-arg https_proxy=$https_proxy -t w3c-server
```

### Run Docker image

Once Docker image is built, we can run it by using the  run command:

```
sudo docker run -it -v $HOME/kuksaval.config:/config -e LOG_LEVEL=ALL amd64/kuksa-val:0.1.1
```
where `$HOME/kuksaval.config` is a directory on your host machine containing the KUKSA.VAL configuration. The directory can be empty, then it will be populated with an exmple configuration during start.

The environment variable `KUKSAVAL_OPTARGS` can be used to add arbitrary command line arguments e.g.

```
sudo docker run -it -v $HOME/kuksaval.config:/config -e LOG_LEVEL=ALL -e KUKSAVAL_OPTARGS="--insecure" amd64/kuksa-val:0.1.1

```

### Advanced docker
Beside all of dependencies already prepared, container image has additional build scripts to help with building of W3C-Server.
Depending on compiler needed, scripts below provide simple initial build configuration to be used and|or extended upon:
 - [docker/build-gcc-default.sh](docker/build-gcc-default.sh),
 - [docker/build-gcc-coverage.sh](docker/build-gcc-coverage.sh),
 - [docker/build-clang-default.sh](docker/build-clang-default.sh),
 - [docker/build-clang-coverage.sh](docker/build-clang-coverage.sh)

In container, scripts are located by default in the `/home/` directory.
Both [build-gcc-default.sh](docker/build-gcc-default.sh) and [build-clang-default.sh](docker/build-clang-default.sh) scripts accept parameters which will be provided to CMake configuration, so user can provide different options to extend default build (e.g add unit test to build).

**Note:** Default path to git repo in the scripts is set to `/home/kuksa.invehicle`. If host path of the git repo, or internally cloned one, is located on different container path, make sure to update scripts accordingly.


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

## Running on AGL on Raspberry Pi 3

* Create an AGL image using the instructions in `agl-kuksa` project.
* Burn the image on to an SD card and boot the image on a Raspi 3.
* w3c-visserver is deployed as a systemd service `w3c-visserver.service` which opens a secure websocket connection on port 8090.

### On first launch

* ssh into the raspi 3 with root.
* Go to `/usr/bin/w3c-visserver` using the ssh connection.
* copy the vss data file https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json into `./usr/bin/w3c-visserver`. By default the AGL build will contain demo cerificates that work with other apps in the repo. You could create your own cerificates and tokens using the instaructions above.
* Reboot the raspi 3
