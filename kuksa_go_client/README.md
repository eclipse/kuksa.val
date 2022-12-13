# Running the KUKSA golang Client

## Execute the example

### Set everything up for the KUKSA.val GO client
- If you do not have GO installed follow this [page](https://go.dev/doc/install) and install v1.18 or above
- If you do not have a protobuf compiler installed execute the following from this directory:
```
> go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
> go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```
Or install the protobuf compiler yourself
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
For Configuration you can use the kuksa-client.json file in this directory or provide the parameters through the command line.
If you want the GO client to work with the KUKSA.val server specify protocol as ws if you want to use the KUKSA.val databroker set protocol to grpc. You can specify the port to look for an instance of KUKSA.val either in the JSON file or through -port on the command line.