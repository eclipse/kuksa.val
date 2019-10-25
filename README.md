# W3C VIS Server Implementation

The implementation is based on the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)

This implementation can also provide additional functionality not (yet) available in (draft) standard documents.

The implementation provides all the major functionality defined in the above standard specification. and also uses JWT Token for permissions handling with decent amount of unit-tests covering all the basic funtions.

## Features
 - Multi-client server implementing Web-Socket platform communication, with support for both secure [SSL] and insecure [plain] connections. Feature status:
 - User authorization based on industry standard RFC 7519 as JSON Web Tokens
 - Optional JSON signing of messages, described in **_JSON signing_** chapter
 - Multi-client server implementing experimental REST API based on standard specification. REST API specification is available as OpenAPI 3.0 definition available in [rest-api.yaml](doc/rest-api.yaml) file.

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

## Dependencies

This project uses components from other 3rd party open-source projects:

| Library       | License       | Description |
| ------------- | ------------- | ----------- |
 [Boost.Beast](https://www.boost.org/doc/libs/1_67_0/libs/beast/doc/html/index.html) | Boost Software license 1.0 | Foundation library for simplified handling of various Web-Socket, HTTP or other protocols with and without security, based on Boost.Asio.
 [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server) _[deprecated]_ | MIT license | Simple implementation of Web-Socket server.
 [jsoncons](https://github.com/danielaparker/jsoncons) | Boost Software license 1.0 | Utility library for handling JSON.
 [jwt-cpp](https://github.com/Thalhammer/jwt-cpp) | MIT license | Utility library for handling JWT tokens.

# Building W3C-Server

[CMake](https://cmake.org/) is tool used to configure, build and package W3C-Server.

## Configure build

Build configuration options of W3C-Server are defined in CMakeLists.txt file.

By changing different option values in CMakeLists.txt file, user can control
different build options of W3C-Server.  
Available build options with optional parameters, if available, are presented below. Default parameters are shown in **bold**:
 - **BUILD_EXE** [**ON**/OFF] - Default build shall produce W3C-Server executable.
   If set to **OFF** W3C-Server shall be built as a library which could be used in another application.
   Eg: could be found in the _vehicle2cloud_ app.
 - **BUILD_TEST_CLIENT** [**ON**/OFF] - Build separate _testclient_ executable. Test client is a utility to test
   Web-Socket request interface of W3C-Server and retrieve responses.
 - **UNIT_TEST** [ON/**OFF**] - If enabled, build shall produce separate _w3c-unit-test_ executable which
   will run existing tests for server implementation.
 - **ADDRESS_SAN** [ON/**OFF**] - If enabled and _Clang_ is used as compiler, _AddressSanitizer_ will be used to build
   W3C-Server for verifying run-time execution.

After changing any of build options, new clean build should be made, as described in **_Building W3C-Server_** chapter.

## Building W3C-Server

To generate new clean build (e.g. after git clone or after changing build configuration options), use standard CMake build order as shown below:

 - Go to W3C-Server directory
```
cd w3c-visserver-api
```
 - Make default build directory where build artifacts will be stored, and move into it
```
mkdir build
cd build
```
 - Invoke CMake pointing to location of CMakeLists.txt file to generate
   Makefile build configuration which will be used to build W3C-Server
```
cmake ..
```
 - Run build W3C-Server. Make parameter '_-j_ ' is optional and allows running parallel build jobs to speed up compilation.
```
make -j
```
If all completes successfully, build artifacts shall be located in 'build' directory.

### JSON signing

JSON Signing has been introduced additionally to sign the JSON response for GET and SUBSCRIBE Response. By default this has been disabled. To enable this feature go to visconf.hpp file and uncomment the line `define JSON_SIGNING_ON`. Please note, JSON signing works only with a valid pair of public / private certificate. For testing, you could create example certificates by following the below steps.
Do not add any passphrase when asked for.

```
ssh-keygen -t rsa -b 4096 -m PEM -f signing.private.key
openssl rsa -in signing.private.key  -pubout -outform PEM -out signing.public.key
```

Copy the files signing.private.key & signing.public.key to the build directory.

The client also needs to validate the signed JSON using the public certificate when JSON signing is enabled in server.

This could also be easily extended to support JSON signing for the requests as well with very little effort.

# Running W3C-Server

Depending on build options and provided parameters, W3C-Server will provide different features.
Chapter **_Parameters_** shall describe different mandatory and optional parameters in more detail.

Default configuration shall provide both Web-Socket and REST API connectivity.

## Web-Socket specific testing

This covers only the basic functions like get, set and getmetadata requests. You coulkd skip this and take a look at the unit-test to get better idea about the implementation.

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
Test page support custom GET, PUT and POST HTTP requests to W3C-Server. Additional benefit is that it can automatically generate JWT token based on input token value and provided Client key which is used in authorization. Note that if users changes Client key, user must also update 'jwt.pub.key' with corresponding public key.

Additional tool which is quite useful is [Swagger](https://editor.swagger.io). It is a dual-use tool which allows for writing OpenAPI specifications, but also generates runnable REST API samples for moslient test endpoints.
Open Swagger editor and import our REST API [definition](./doc/rest-api.yaml) file. Swagger shall generate HTML page with API documentation. When one of the endpoints is selected, 'try' button appears which allows for making REST requests directly to running W3C-Server.

## Parameters
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
- **--cert-path** [mandatory] - Directory path where 'Server.pem', 'Server.key' and 'jwt.pub.key' are located. Server demo certificates are located in [this](https://github.com/eclipse/kuksa.invehicle/tree/master/examples/demo-certificates) directory of git repo. Certificates from 'demo-certificates' are automatically copied to build directory, so invoking '_--cert-path=._' should be enough when demo certificates are used.  
If user needs to use or generate their own certificates, see chapter **_Certificates_** for more details.  
For authorizing client, file 'jwt.pub.key' contains public key used to verify that JWT authorization token is valid. To generated different 'jwt.pub.key' file, see chapter **_Permissions_** for more details.
- **--insecure** [optional] - By default, W3C-Server shall accept only SSL (TLS) secured connections. If provided, W3C-Server shall also accept plain un-secured connections for Web-Socket and REST API connections, and also shall not fail connections due to self-signed certificates.
- **--wss-server** [optional][deprecated] - By default, W3C-Server uses Boost.Beast as default connection handler. If provided, W3C-Server shall use deprecated Simple Web-Socket Server, without REST API support.
- **--address** [optional] - If provided, W3C-Server shall use different server address than default _'localhost'_.
- **--port** [optional] - If provided, W3C-Server shall use different server port than default '8090' value.
- **--log-level** [optional] - Enable selected log level value. To allow for different log level combinations, parameter can be provided multiple times with different log level values.

# How tos

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

## Permissions

The W3C-Server needs authorization JWT Token to allow access to server side resources. You can create a dummy JWT Token from https://jwt.io/. Use the RSA256 algorithm from the drop down and enter valid "iat" and "exp" data and set "iss : kuksa" and generate a JWT. Once the JWT is generated on the left side. Copy the Public key from the Text box on the right side to a file and rename the field to jwt.pub.key and copy the file to  `w3c-visserver/build` folder. Also store the JWT token somewhere so that you could pass the Token to the server for authentication.

![Alt text](./doc/pictures/jwt.png?raw=true "jwt")

Permissions can be granted by modifying the JSON Claims.

1. The JWT Token should contain a "w3c-vss" claim.
2. Under the "w3c-vss" claim the permissions can be granted using key value pair. The key should be the path in the signal tree and the value should be strings with "r" for READ-ONLY, "w" for WRITE-ONLY and "rw" or "wr" for READ-AND-WRITE permission. See the image above.
3. The permissions can contain wild-cards. For eg "Signal.OBD.\*" : "rw" will grant READ-WRITE access to all the signals under Signal.OBD.
4. The permissions can be granted to a branch. For eg "Signal.Vehicle" : "rw" will grant READ-WRITE access to all the signals under Signal.Vehicle branch.

## Running on AGL on Raspberry Pi 3

* Create an AGL image using the instructions in `agl-kuksa` project.
* Burn the image on to an SD card and boot the image on a Raspi 3.
* w3c-visserver is deployed as a systemd service `w3c-visserver.service` which opens a secure websocket connection on port 8090.

### On first launch

* ssh into the raspi 3 with root.
* Go to `/usr/bin/w3c-visserver` using the ssh connection.
* copy the vss data file https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json into `./usr/bin/w3c-visserver`. By default the AGL build will contain demo cerificates that work with other apps in the repo. You could create your own cerificates and tokens using the instaructions above.
* Reboot the raspi 3
