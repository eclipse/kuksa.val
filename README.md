# W3C VIS Server Implementation

The implementation is based on the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)


The implementation at the moment is in a nacent state and includes only the basic functionalities mentioned in the specification. At the moment, the security related functions have not been touched upon but will be updated shortly. This project uses components from other open source projects namely

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

# How to run
This application needs the input vss data to create the tree structure. The input file can be taken from https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json. Clone the files and place them in the build folder where the executables are built. Keep the names of the files the same.
Add the files to the location where the application executable is run from.


# How tos
Create a self-signed certificate using the steps mention [here]( https://kb.op5.com/pages/viewpage.action?pageId=19073746#sthash.GHsaFkZe.WDGgcOja.dpbs).

Follow the instructions create the private and public key files. Follow the process create 2 sets of key pairs using the same CA ( Certificate authority ) and at the end rename the MyRoot.key to CA.key. Use CA.key and one set of keys (.key and .pem) on the client side ( rename files to Client.pem and Client.key). and use the other set of keys (.pem and .key) on the server side ( rename files to Server.pem and Server.key).

Now enable `BUILD_EXE` and `BUILD_TEST_CLIENT` flags by changing to ON in w3cvisserver/CMakeLists.txt.

Now build using the commands in How to build section.

Once the apps are built, copy the server.crt and server.key files to the `w3c-visserver/build` folder. Also copy the  https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.csv and https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json files into the `w3c-visserver/build` folder.

In this case the server and the client are built on the same folder, hence copy the generate Server.pem, Server.key ,CA.key, Client.key and  Client.pem into `w3c-visserver/build` folder.

The w3c-visserver needs authentification Token to allow access to server side resources. You can create a dummy JWT Token from https://jwt.io/. Use the RSA256 algorithm from the drop down and enter valid "iat" and "exp" data and set "iss : kuksa" and generate a JWT. Once the JWT is generated on the left side. Copy the Public key from the Text box on the right side to a file and rename the fiel to jwt.pub.key and copy the file to  `w3c-visserver/build` folder. Also store the JWT token somewhere so that you could pass the Token to the server for authentication.

![Alt text](./pictures/jwt.png?raw=true "jwt")

Now the apps are ready for testing. Run w3c-visserver using `./w3c-visserver` command and then in a separate terminal start testclient using `./testclient`.

Testclient should connect to the w3c-visserver and promt a message as below
![Alt text](./pictures/test1.png?raw=true "test1")

Authenticate with the server using the JWT token
![Alt text](./pictures/test4.png?raw=true "test4")

Enter the vss path and function as set and a dummy integer value.
![Alt text](./pictures/test2.png?raw=true "test2")

Enter the same vss path as above and fuction as get. You should receive the previously set value in the JSON response. 
![Alt text](./pictures/test3.png?raw=true "test3")


# Implementation

| Feature       | Status        |
| ------------- | ------------- |
| GET/SET       | :heavy_check_mark:|
| PUB/SUB  | :heavy_check_mark: |
| GETMETA  | :heavy_check_mark: |
| Secure WebSocket  | :heavy_check_mark: |   
| Authentification  | :heavy_check_mark: |

## Running on AGL on Raspberry Pi 3

* Create an AGL image using the instructions in `agl-kuksa` project.
* Burn the image on to an SD card and boot the image on a Raspi 3.
* w3c-visserver is deployed as a systemd service `w3c-visserver.service` which opens a secure websocket connection on port 8090.


#### On first launch

* ssh into the raspi 3 with root.
* Go to `/usr/bin/w3c-visserver` using the ssh connection.
* copy the vss data file https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json into `./usr/bin/w3c-visserver`. By default the AGL build will contain demo cerificates that work with other apps in the repo. You could create your own cerificates and tokens using the instaructions above.
* Reboot the raspi 3

