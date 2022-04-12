# Running the Kuksa golang Client

## Prerequisites
- golang toolchain ([v1.18 or above](https://go.dev/doc/install))

## Execute the example
### Start kuksa.val Server
- Build kuksa.val and start the server
```
> cd <repo>/build/src/
> ./kuksa-val-server
```

- Alternatively, start the appropriate docker container.
```
> docker run -it <arch>/kuksa-val
```

### Run the kuksa.val go client
- Navigate to this directory in the repository.
- You can build and execute the example as following
```
> go build .
> ./kuksa_go_client
```
- Alternatively, execute
```
> go run .
```
