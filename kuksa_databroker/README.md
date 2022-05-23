# Kuksa Data Broker

- [Kuksa Databroker](#kuksa-data-broker)
  - [Intro](#intro)
  - [Interface](#interface)
  - [Relation to the COVESA Vehicle Signal Specification (VSS)](#relation-to-the-covesa-vehicle-signal-specification-vss)
  - [Building](#building)
    - [Build all](#build-all)
    - [Build all release](#build-all-release)
  - [Running](#running)
    - [Broker](#broker)
    - [Test the broker - run client/cli](#test-the-broker---run-clientcli)
    - [Kuksa Data Broker Query Syntax](#databroker-query-syntax)
    - [Configuration](#configuration)
    - [Build and run databroker container](#build-and-run-databroker-container)
  - [Limitations](#limitations)
  - [GRPC overview](#grpc-overview)
  - [Data Privacy Information](#data-privacy-information)

## Intro

Kuksa Data Broker is a GRPC service acting as a broker of vehicle data / data points / signals.

## Interface

The main interface, used by clients is defined as follows (see [file](proto/sdv/databroker/v1/broker.proto) in the proto folder):

```protobuf
service Broker {
    rpc GetDatapoints(GetDatapointsRequest) returns (GetDatapointsReply);
    rpc Subscribe(SubscribeRequest) returns (stream Notification);

    rpc GetMetadata(GetMetadataRequest) returns (GetMetadataReply);
}
```

There is also a [Collector](proto/sdv/databroker/v1/collector.proto) interface which is used by data point providers to feed data into the broker.

```protobuf
service Collector {
    rpc RegisterDatapoints(RegisterDatapointsRequest) returns (RegisterDatapointsReply);

    rpc UpdateDatapoints(UpdateDatapointsRequest) returns (UpdateDatapointsReply);

    rpc StreamDatapoints(stream StreamDatapointsRequest) returns (stream StreamDatapointsReply);
}
```

## Relation to the COVESA Vehicle Signal Specification (VSS)

The data broker is designed to support data entries and branches as defined by [VSS](https://covesa.github.io/vehicle_signal_specification/).

In order to generate metadata from a VSS specification that can be loaded by the data broker, it's possible to use the `vspec2json.py` tool
that's available in the [vss-tools](https://github.com/COVESA/vss-tools) repository. E.g.

```shell
./vss-tools/vspec2json.py -I spec spec/VehicleSignalSpecification.vspec vss.json
```

The resulting vss.json can be loaded at startup my supplying the data broker with the command line argument:

```shell
--metadata vss.json
```

## Building

Prerequsites:
- [Rust](https://www.rust-lang.org/tools/install)
- Linux

### Build all

`cargo build --examples --bins`

### Build all release

`cargo build --examples --bins --release`

## Running

### Databroker
Run the broker with:

`cargo run --bin databroker`

Get help, options and version number with:

`cargo run --bin databroker -- -h`

```shell
Kuksa Data Broker

USAGE:
    databroker [OPTIONS]

OPTIONS:
        --address <ADDR>    Bind address [default: 127.0.0.1]
        --port <PORT>       Bind port [default: 55555]
        --metadata <FILE>   Populate data broker with metadata from file [env:
                            KUKSA_DATA_BROKER_METADATA_FILE=]
        --dummy-metadata    Populate data broker with dummy metadata
    -h, --help              Print help information
    -V, --version           Print version information

```

### Test the databroker - run client/cli

Run the cli with:

`cargo run --bin databroker-cli`

To get help and an overview to the offered commands, run the cli and type :

```shell
client> help
```


If server wasn't running at startup

```shell
client> connect
```


The server holds the metadata for the available properties, which is fetched on client startup.
This will enable `TAB`-completion for the available properties in the client. Run "metadata" in order to update it.


Get data points by running "get"
```shell
client> get Vehicle.ADAS.CruiseControl.Error
-> Vehicle.ADAS.CruiseControl.Error: NotAvailable
```

Set data points by running "set"
```shell
client> set Vehicle.ADAS.CruiseControl.Error Nooooooo!
-> Ok
```

### Data Broker Query Syntax

Detailed information about the databroker rule engine can be found in [QUERY.md](doc/QUERY.md)


You can try it out in the client using the subscribe command in the client:

```shell
client> subscribe
SELECT
  Vehicle.ADAS.ABS.Error
WHERE
  Vehicle.ADAS.ABS.IsActive 

-> status: OK
```

### Configuration

| parameter      | default value | cli parameter    | environment variable              | description                                  |
|----------------|---------------|------------------|-----------------------------------|----------------------------------------------|
| metadata       | <no active>   | --metadata       | KUKSA_DATA_BROKER_METADATA_FILE | Populate data broker with metadata from file |
| dummy-metadata | <no active>   | --dummy-metadata | <no active>                       | Populate data broker with dummy metadata     |
| listen_address | "127.0.0.1"   | --address        | KUKSA_DATA_BROKER_ADDR          | Listen for rpc calls                         |
| listen_port    | 55555         | --port           | KUKSA_DATA_BROKER_PORT          | Listen for rpc calls                         |

To change the default configuration use the arguments during startup see [run section](#running) or environment variables.

### Build and run databroker

To build the release version of databroker, run the following command:

```shell
RUSTFLAGS='-C link-arg=-s' cargo build --release --bins --examples
```
Or use following commands for aarch64
```
cargo install cross

RUSTFLAGS='-C link-arg=-s' cross build --release --bins --examples --target aarch64-unknown-linux-gnu
```
Build tar file from generated binaries.

```shell
# For amd64
tar -czvf databroker_x86_64.tar.gz \
    target/release/databroker \
    target/release/databroker-cli \
    target/release/examples/perf_setter \
    target/release/examples/perf_subscriber
```
```shell
# For aarch64
tar -czvf databroker_aarch64.tar.gz \
    target/aarch64-unknown-linux-gnu/release/databroker-cli \
    target/aarch64-unknown-linux-gnu/release/databroker \
    target/aarch64-unknown-linux-gnu/release/examples/perf_setter \
    target/aarch64-unknown-linux-gnu/release/examples/perf_subscriber
```
To build the image execute following commands from root directory as context.
```shell
docker build -f kuksa_databroker/Dockerfile -t databroker:<tag> .
```

Use following command if buildplatform is required
```shell
DOCKER_BUILDKIT=1 docker build -f kuksa_databroker/Dockerfile -t databroker:<tag> .
```

The image creation may take around 2 minutes.
After the image is created the databroker container can be ran from any directory of the project:
```shell
#By default the container will execute the ./databroker command.
docker run --rm -it  -p 55555:55555/tcp databroker
```
To run any specific command, just append you command at the end.

```shell
docker run --rm -it  -p 55555:55555/tcp databroker <command>
```

## Limitations

- Arrays are not supported in conditions as part of queries (i.e. in the WHERE clause).
- Arrays are not supported by the cli (except for displaying them)

## GRPC overview
Vehicle data broker uses GRPC for the communication between server & clients.

A GRPC service uses `.proto` files to specify the services and the data exchanged between server and client.
From this `.proto`, code is generated for the target language (it's available for C#, C++, Dart, Go, Java, Kotlin, Node, Objective-C, PHP, Python, Ruby, Rust...)

This implementation uses the default GRPC transport HTTP/2 and the default serialization format protobuf. The same `.proto` file can be used to generate server skeleton and client stubs for other transports and serialization formats as well.

HTTP/2 is a binary replacement for HTTP/1.1 used for handling connections / multiplexing (channels) & and providing a standardized way to add authorization headers for authorization & TLS for encryption / authentication. It support two way streaming between client and server.

## Data Privacy Information

Your privacy is important to us.
The following Information is to provide you with information relevant to data protection in order to be able to use the software in a data protection compliant manner.
It is provided as an information source for your solution-specific data protection and data privacy topics.
This is not intended to provide and should not be relied on for legal advice.

### Your Role

First things first: when you choose and use our software, you are most likely acting in the role of data controller, if personal related data is being processed.
Therefore, you must ensure that the processing of personal data complies with the respective local legal requirements, e.g. when processing data within the scope of General Data Protection Regulation (GDPR) the legal requirements for a controller from the GDPR.

### Where may the processing of personal related data be relevant?

When using our software in combination with other software components, personal data or data categories may be collected for the purpose of developing, testing and running in-vehicle applications (Vehicle Apps).
Possible examples are the vehicle identification number (VIN), the number plate, GPS data, video data, audio data, or other measurement data.
You can determine which data or data categories are collected when configuring the software.
These data are stored in volatile memory and are deleted by shutting down the system.
You are responsible for the compliant handling of the data in accordance with the applicable local law.

### What have we done to make the software data protection friendly?

This section describes the measures taken to integrate the requirements of the data protection directly into the software development.
The technical measures described below follow a "privacy by design" approach.

- Deletion possibility: The software does not save data permanently since it uses only volatile memory. All collected or processed data can be deleted by rebooting the host hardware.