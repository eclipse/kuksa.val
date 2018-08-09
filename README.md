# W3C VIS Server Implementation

The implementation is based on the [W3C Vehicle Information Service Specification](https://www.w3.org/TR/2018/CR-vehicle-information-service-20180213/)


The implementation at the moment is in a nacent state and includes only the basic functionalities mentioned in the specification. At the moment, the security related functions have not been touched upon but will be updated shortly. This project uses components from other open source projects namely

1. [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server) which is under MIT license.
2. [jsoncons](https://github.com/danielaparker/jsoncons) which is under Boost Software License.


# How to build
w3c-visserver can be built as a library which could be used in another application. Eg: could be found in the vehicle2cloud app.
```
cd w3c-visserver
mkdir build
cd build
cmake ..
make
```

# How to run
This application needs the input vss data to create the tree structure. The input files can be taken from https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.csv and https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json. Clone the files and place them in the build folder where the executables are built. Keep the names of the files the same.
Add the files to the location where the application executable is run from.


# Test Secure Websocket connection
Create a self-signed certificate using the steps mention [here]( http://www.akadia.com/services/ssh_test_certificate.html).

Follow the instructions till step 4, this will help you create server.key and server.crt files.

Now enable `BUILD_EXE` and `BUILD_TEST_CLIENT` flags by changing to ON in w3cvisserver/CMakeLists.txt.

Now build using the commands in How to build section.

Once the apps are built, copy the server.crt and server.key files to the `w3c-visserver/build` folder. Also copy the  https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.csv and https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json files into the `w3c-visserver/build` folder.

Now the apps are ready for testing. Run w3c-visserver using `./w3c-visserver` command and then in a separate terminal start testclient using `./testclient`.

Testclient should connect to the w3c-visserver and promt a message as below
![Alt text](./pictures/test1.png?raw=true "test1")

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
| Authentification  | :heavy_multiplication_x: |

