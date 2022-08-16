# Kuksa-val-server Unit Test

### Requires Boost version > 1.59

Enable set(UNIT_TEST ON) in CMakeLists.txt.

```
cd kuksa-val-server

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
Before you are using grpc_cli you have to start the server in insecure mode.

```
    cd build/src
    ./kuksa-val-server --insecure

```
With ./grpc_cli ls you can expose the services and their rpc methods and proto messages.
```
    cd grpc/cmake/build
    // all services
    ./grpc_cli ls localhost:50051 
    // the rpc methodes of the viss_client service
    ./grpc_cli ls localhost:50051 kuksa.viss_client -l
    // properties of a rpc method
    ./grpc_cli type localhost:50051 kuksa.getMetaDataRequest
```
With ./grpc_cli call you can make a call and specify parameters.

```
    // call GetMetaData rpc method with parameters
    ./grpc_cli call localhost:50051 GetMetaData "path_:'Vehicle.Speed', reqID_:'123456'"
```

With this the interface could be tested manually.
