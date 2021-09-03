# W3C-VISS Unit Test

### Requires Boost version > 1.59

Copy the vss signal tree json manually from [here](https://github.com/GENIVI/vehicle_signal_specification/blob/master/vss_rel_1.0.json) to the build directory once you have build the unit test binary using the following instaructions.

Enable set(UNIT_TEST ON) in CMakeLists.txt under w3v-visserver-api folder.

```

mkdir build

cd build

cmake .. -DBUILD_UNIT_TEST=ON

make

```
you can execute using

```
cd test/unit-test

./kuksaval-unit-test

```

Obviously, there should be no errors :)

There are only unit tests for the gRPC Client Business Logic. For testing the Server use grpc_cli. Follow the instructions from https://github.com/grpc/grpc/blob/master/doc/command_line_tool.md.
Before you start using grpc_cli you have to start the server.
With ./grpc_cli ls you can expose the services and their rpc methods and proto messages.
With ./grpc_cli call you can make a call and specify parameters.
With this the interface could be tested manually.