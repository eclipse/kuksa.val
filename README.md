# W3C VIS Server Implementation

The implementation is based on the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)


The implementation provides all the major funtionality defined in the above specification and also uses JWT Token for permissions handling with decent amount of uni-tests covering all the basic funtions. This project uses components from other open source projects namely

1. [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server) which is under MIT license.
2. [jsoncons](https://github.com/danielaparker/jsoncons) which is under Boost Software license.
3. [jwt-cpp](https://github.com/Thalhammer/jwt-cpp)which is under MIT license. 


# How to build
w3c-visserver can be built as a library which could be used in another application. Eg: could be found in the vehicle2cloud app.
```
cd w3c-visserver-api
mkdir build
cd build
cmake ..
make
```

JSON Signing has been introduced additionally to sign the JSON response for GET and SUBSCRIBE Response. By default this has been disabled. To enable this feature go to visconf.hpp file and uncomment the line `define JSON_SIGNING_ON`. Please note, JSON signing works only with a valid pair of public / private certificate. For testing, you could create example certificates by following the below steps.
Do not add any passphrase when asked for.

```
ssh-keygen -t rsa -b 4096 -m PEM -f signing.private.key 
openssl rsa -in signing.private.key  -pubout -outform PEM -out signing.public.key
```

Copy the files signing.private.key & signing.public.key to the build directory.

The client also needs to validate the signed JSON using the public certificate when JSON signing is enabled in server.

This could also be easily extended to support JSON signing for the requests as well with very little effort.


# How to run
This application needs the input vss data to create the tree structure. The input file can be taken from https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json. Clone the files and place them in the build folder where the executables are built. Keep the names of the files the same.
Add the files to the location where the application executable is run from.


# How tos

Demo Certificates are available in the examples/demo-certificates folder and these certs are automatically copied on building the apps and api. In case you need to create new certs follow the steps below, otherwise skip the steps below.

### Create PKI certificates

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

### Build W3C-Server

Now enable `BUILD_EXE` and `BUILD_TEST_CLIENT` flags by changing to ON in w3cvisserver/CMakeLists.txt.

Now build using the commands in How to build section.

Once the apps are built, copy the server.crt and server.key files to the `w3c-visserver/build` folder. Also copy the  https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json files into the `w3c-visserver/build` folder.

In this case the server and the client are built on the same folder, hence copy the generate Server.pem, Server.key ,CA.key, Client.key and  Client.pem into `w3c-visserver/build` folder.

#### Permissions

The w3c-visserver needs authentification Token to allow access to server side resources. You can create a dummy JWT Token from https://jwt.io/. Use the RSA256 algorithm from the drop down and enter valid "iat" and "exp" data and set "iss : kuksa" and generate a JWT. Once the JWT is generated on the left side. Copy the Public key from the Text box on the right side to a file and rename the field to jwt.pub.key and copy the file to  `w3c-visserver/build` folder. Also store the JWT token somewhere so that you could pass the Token to the server for authentication.

![Alt text](./pictures/jwt.png?raw=true "jwt")

Permissions can be granted by modifying the JSON Claims.

1. The JWT Token should contain a "w3c-vss" claim.
2. Under the "w3c-vss" claim the permissions can be granted using key value pair. The key should be the path in the signal tree and the value should be strings with "r" for READ-ONLY, "w" for WRITE-ONLY and "rw" or "wr" for READ-AND-WRITE permission. See the image above.
3. The permissions can contain wild-cards. For eg "Signal.OBD.*" : "rw" will grant READ-WRITE access to all the signals under Signal.OBD.
4. The permissions can be granted to a branch. For eg "Signal.Vehicle" : "rw" will grant READ-WRITE access to all the signals under Signal.Vehicle branch.

### Test Run

This covers only the basic functions like get, set and getmetadata requests. You coulkd skip this and take a look at the unit-test to getter idea about the implementation.

You could also checkout the in-vehicle apps in the [kuksa.apps](https://github.com/eclipse/kuksa.apps) repo which work with the server.

Now the apps are ready for testing. Run w3c-visserver using `./w3c-visserver` command and then in a separate terminal start testclient using `./testclient`.

Testclient should connect to the w3c-visserver and promt a message as below
![Alt text](./pictures/test1.png?raw=true "test1")

Authenticate with the server using the JWT token
![Alt text](./pictures/test4.png?raw=true "test4")

Enter the vss path and function as set and a dummy integer value.
![Alt text](./pictures/test2.png?raw=true "test2")

Enter the same vss path as above and fuction as get. You should receive the previously set value in the JSON response. 
![Alt text](./pictures/test3.png?raw=true "test3")





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





# Implementation

| Feature       | Status        |
| ------------- | ------------- |
| GET/SET       | :heavy_check_mark:|
| PUB/SUB  | :heavy_check_mark: |
| GETMETA  | :heavy_check_mark: |
| Secure WebSocket  | :heavy_check_mark: |   
| Authentification  | :heavy_check_mark: |
| JSON signing    | :heavy_check_mark:  |

## Running on AGL on Raspberry Pi 3

* Create an AGL image using the instructions in `agl-kuksa` project.
* Burn the image on to an SD card and boot the image on a Raspi 3.
* w3c-visserver is deployed as a systemd service `w3c-visserver.service` which opens a secure websocket connection on port 8090.


#### On first launch

* ssh into the raspi 3 with root.
* Go to `/usr/bin/w3c-visserver` using the ssh connection.
* copy the vss data file https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json into `./usr/bin/w3c-visserver`. By default the AGL build will contain demo cerificates that work with other apps in the repo. You could create your own cerificates and tokens using the instaructions above.
* Reboot the raspi 3

