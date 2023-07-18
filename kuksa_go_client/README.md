# Running the KUKSA golang Client

## Execute the example

### Set everything up for the KUKSA.val GO client
- If you do not have GO installed follow this [page](https://go.dev/doc/install) and install v1.18 or above
- If you do not have a protobuf compiler installed execute the following from this directory:
```
> go run protocInstall/protocInstall.go
```
Or install the protobuf compiler yourself(https://grpc.io/docs/protoc-installation/)
- Add the protobuf compiler (e.g. HOME_DIR/protoc/bin) to your PATH variable. For example for linux do:
```
> export PATH=$PATH:$HOME/protoc/bin
```
- Run the following command to install the needed GO protocol buffers plugins:
```
> go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
> go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```
The plugins will be installed in $GOBIN, defaulting to $GOPATH/bin which is default HOME_DIR/go/bin. It must be in your $PATH for the protocol compiler protoc to find it.
For linux execute:
```
> export PATH=$PATH:$HOME/go/bin
```
- Then execute
```
> go generate .
```
if you encounter a problem, you have to give the protoc executable the right to be executed e.g in Linux run
```
> sudo chmod +x <HOME_DIR>/protoc/bin/protoc
```
### Run the KUKSA.val GO client
#### Start KUKSA.val Server or Databroker
- Build kuksa.val and start the server
```
> cd kuksa.val/kuksa-val-server/build/src/
> ./kuksa-val-server
```
- Alternatively, start the appropriate docker container.
```
> docker run -it --rm --net=host -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/kuksa-val:master
```
- Build and run KUKSA.val Databroker by executing:
```
> cargo run --bin databroker
```
- Alternatively, start the apropriate docker container.
```
> docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master
```
- To run the GO Client execute:
```
> go build .
> go run .
```
- Alternatively, execute:
```
> ./kuksa_go_client
```

### Configuration of the KUKSA.val GO client
The GO clients reads the configuration file `kuksa-client.json`. In this repository example configurations for both
KUKSA.val Databroker (`kuksa-client-grpc.json`) and KUKSA.val Server (`kuksa-client-ws.json`) exists.
The file `kuksa-client.json` is by default linked to `kuksa-client-grpc.json`.

For using the GO client with the kuksa.val server set protocol = ws and for a connection to kuksa.val databroker set protocol = grpc. On the command line it's available through -protocol ws/grpc.

### Dependency updates

If dependencies needs to be updated the following commands can be used:

```
go generate .
go get -u
go mod tidy
```

This will update `go.mod`and `go.sum`.

## Linters

Our Continuous Integration verifies that the code pass the [Golang Linter](https://golangci-lint.run/usage/install).
To avoid failing PR builds it is recommended to run the linter manually before creating a Pull Request.
