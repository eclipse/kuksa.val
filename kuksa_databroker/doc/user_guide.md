<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a name="top"></a>
# Eclipse Kuksa.val&trade; Databroker User Guide

The following sections provide information for running and configuring Databroker as well as information necessary for developing client applications invoking Databroker's external API.

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#getting-help">Getting Help</a></li>
    <li><a href="#running-databroker">Running Databroker</a></li>
    <li><a href="#enabling-authorization">Enabling Authorization</a></li>
    <li><a href="#enabling-tLS">Enabling TLS</a></li>
    <li><a href="#query-syntax">Query Syntax</a></li>
    <li><a href="#using-custom-vss-data-entries">Using Custom VSS Data Entries</a></li>
    <li><a href="#configuration-reference">Configuration Reference</a></li>
    <li><a href="#signal-change-types">Signal Change Types</a></li>
    <li><a href="#api">API</a></li>
    <li><a href="#known-limitations">Known Limitations</a></li>
  </ol>
</details>

## Getting help

Get help, options and version number with:

```sh
docker run --rm -it ghcr.io/eclipse/kuksa.val/databroker:master -h
```
```console
Usage: databroker [OPTIONS]

Options:
      --address <IP>            Bind address [env: KUKSA_DATA_BROKER_ADDR=] [default: 127.0.0.1]
      --port <PORT>             Bind port [env: KUKSA_DATA_BROKER_PORT=] [default: 55555]
      --vss <FILE>              Populate data broker with VSS metadata from (comma-separated) list of files [env: KUKSA_DATA_BROKER_METADATA_FILE=]
      --jwt-public-key <FILE>   Public key used to verify JWT access tokens
      --disable-authorization   Disable authorization
      --insecure                Allow insecure connections
      --tls-cert <FILE>         TLS certificate file (.pem)
      --tls-private-key <FILE>  TLS private key file (.key)
  -h, --help                    Print help
  -V, --version                 Print version
```

<p align="right">(<a href="#top">back to top</a>)</p>

## Running Databroker

Before starting Databroker you must decide if you want to use TLS for incoming connections or not. It is recommended to use TLS which is enabled by providing a private key with `--tls-private-key` and a server certificate with `--tls-cert`. If you do not provide those options, Databroker will only accept insecure connections. The default behavior may change in the future, so if you want insecure connections it is recommended to use the `--insecure` argument.

```sh
docker run --rm -it -p 55555:55555 ghcr.io/eclipse/kuksa.val/databroker:master --insecure
```

> :warning: **Warning**: Default port not working on Mac OS
>
> On several versions of Mac OS applications cannot bind to port `55555`. Databroker needs to be configured to bind to a different (local) port in such cases:
>
> ```sh
> docker run --rm -it -p 55556:55555 ghcr.io/eclipse/kuksa.val/databroker:master --insecure
> ```
>
> Please refer to [this support forum post](https://developer.apple.com/forums/thread/671197) for additional information.

<p align="right">(<a href="#top">back to top</a>)</p>

## Enabling Authorization

Kuksa.val Databroker supports authorizing client requests based on JSON Web Tokens (JWT) provided by clients in request messages. This requires configuration of a PEM file containing the public key that should be used for verifying the tokens' signature.

The Kuksa.val repository contains example keys and JWTs in the *kuksa_certificates* and *jwt* folders respectively which can be used for testing purposes. In order to run the commands below, the repository first needs to be cloned to the local file system:

```shell
git clone https://github.com/eclipse/kuksa.val.git
```

The Databroker can then be started with support for authorization from the repository root folder:

```shell
# in repository root
docker run --rm -it --name Server --network kuksa -v ./kuksa_certificates:/opt/kuksa ghcr.io/eclipse/kuksa.val/databroker:master --insecure --jwt-public-key /opt/kuksa/jwt/jwt.key.pub
```

The CLI can then be configured to use a corresponding token when connecting to the Databroker:

```shell
# in repository root
docker run --rm -it --network kuksa -v ./jwt:/opt/kuksa ghcr.io/eclipse/kuksa.val/databroker-cli:master --server Server:55555 --token-file /opt/kuksa/read-vehicle-speed.token
```

The token contains a claim that authorizes the client to read the *Vehicle.Speed* signal only.
Consequently, checking if the vehicle cabin's dome light is switched on fails:

```sh
get Vehicle.Cabin.Light.IsDomeOn
```
```console
[get]  OK
Vehicle.Cabin.Light.IsDomeOn: ( AccessDenied )
```

Retrieving the vehicle's current speed succeeds but yields no value because no value has been set yet:

```sh
get Vehicle.Speed
```
```console
[get]  OK
Vehicle.Speed: ( NotAvailable )
```
<p align="right">(<a href="#top">back to top</a>)</p>

## Enabling TLS

Kuksa.val Databroker also supports using TLS for encrypting the traffic with clients. This requires configuration of both a PEM file containing the server's private key as well as a PEM file containing the server's X.509 certificate.

The command below starts the Databroker using the example key and certificate from the Kuksa.val repository:

```sh
# in repository root
docker run --rm -it --name Server --network kuksa -v ./kuksa_certificates:/opt/kuksa ghcr.io/eclipse/kuksa.val/databroker:master --tls-cert /opt/kuksa/Server.pem --tls-private-key /opt/kuksa/Server.key
```

The CLI can then be configured to use a corresponding trusted CA certificate store when connecting to the Databroker:

```shell
# in repository root
docker run --rm -it --network kuksa -v ./kuksa_certificates:/opt/kuksa ghcr.io/eclipse/kuksa.val/databroker-cli:master --server Server:55555 --ca-cert /opt/kuksa/CA.pem
```
<p align="right">(<a href="#top">back to top</a>)</p>

## Query Syntax

Clients can subscribe to updates of data entries of interest using an SQL-based [query syntax](./QUERY.md).

You can try it out using the `subscribe` command in the client:

```shell
subscribe
SELECT
  Vehicle.ADAS.ABS.IsError
WHERE
  Vehicle.ADAS.ABS.IsEngaged
```

```console
[subscribe]  OK
Subscription is now running in the background. Received data is identified by [1].
```
<p align="right">(<a href="#top">back to top</a>)</p>

## Using Custom VSS Data Entries

Kuksa.val Databroker supports management of data entries and branches as defined by the [Vehicle Signal Specification](https://covesa.github.io/vehicle_signal_specification/).

In order to generate metadata from a VSS specification that can be loaded by the data broker, it's possible to use the `vspec2json.py` tool
that's available in the [vss-tools](https://github.com/COVESA/vss-tools) repository:

```shell
./vss-tools/vspec2json.py -I spec spec/VehicleSignalSpecification.vspec vss.json
```

The Databroker can be configured to load the resulting `vss.json` file at startup:

```shell
docker run --rm -it -p 55555:55555 ghcr.io/eclipse/kuksa.val/databroker:master --insecure --vss vss.json
```
<p align="right">(<a href="#top">back to top</a>)</p>

## Signal Change Types

Internally, databroker knows different change types for VSS signals. There are three change-types

 - **Continuos**: This are usually sensor values that are continuos, such as vehicle speed. Whenever a continuos signal is updated by a provider, all subscribers are notified.
 - **OnChange**: This are usually signals that indicate a state, for example whether a door is open or closed. Even if this data is updated regularly by a provider, subscribers are only notified if the the value actually changed.
 - **Static**: This are signals that you would not expect to change during one ignition cycle, i.e. if an application reads it once, it could expect this signal to remain static during the runtime of the application. The VIN might be an example for a static signal. Currently, in the implementation subscribing `static` signals behaves exactly the same as `onchange` signals.

Currently the way signals are classified depends on databroker version.

Up until version 0.4.1 (including)
 - All signals where registered as **OnChange**

Starting from version 0.4.2, if nothing else is specified
 - All signals that are of VSS type `sensor` or `actuator` are registered as change type `continuos`
 - All attributes are registered as change type `static`

VSS itself has no concept of change types, but you can explicitly configure this behavior on vss level with the custom extended attribute `x-kuksa-changetype`, where valid values are `continuos`, `onchange`, `static`.

Check these `.vspec` snippets as example

```yaml
VehicleIdentification.VIN:
  datatype: string
  type: attribute
  x-kuksa-changetype: static
  description: 17-character Vehicle Identification Number (VIN) as defined by ISO 3779.

Vehicle.Speed:
  datatype: float
  type: sensor
  unit: km/h
  x-kuksa-changetype: continuos
  description: Vehicle speed.

Vehicle.Cabin.Door.Row1.Left.IsOpen:
  datatype: boolean
  type: actuator
  x-kuksa-changetype: onchange
  description: Is door open or closed
```

The change types currently apply on *current* values, when subscribing to a *target value*, as an actuation provider would do, any set on the target value is propagated just like in `continuos` mode, even if a datapoint (and thus its current value behavior) is set to `onchange` or `static`. The idea here is, that a "set" by an application is the intent to actuate something (maybe a retry even), and should thus always be forwarded to the provider.


## Configuration Reference

The default configuration can be overridden by means of setting the corresponding environment variables and/or providing options on the command line as illustrated in the previous sections.

| CLI option          | Environment Variable              | Default Value | Description |
|---------------------|-----------------------------------|---------------|-------------|
|`--vss`,<br>`--metadata`| `KUKSA_DATA_BROKER_METADATA_FILE` |               | Populate data broker with metadata from file |
|`--address`          | `KUKSA_DATA_BROKER_ADDR`          | `127.0.0.1`   | Listen for rpc calls                         |
|`--port`             | `KUKSA_DATA_BROKER_PORT`          | `55555`       | Listen for rpc calls                         |
|`--jwt-public-key`   |                                   |               | Public key used to verify JWT access tokens |
|`--tls-cert`         |                                   |               | TLS certificate file (.pem) |
|`--tls-private-key`  |                                   |               | TLS private key file (.key) |
|`--insecure`         |                                   |               | Allow insecure connections (default unless `--tls-cert` and `--tls-private-key` options are provided) |

<p align="right">(<a href="#top">back to top</a>)</p>

## API

Kuksa.val Databroker provides [gRPC](https://grpc.io/) based API endpoints which can be used by
clients to interact with the server.

gRPC services are specified by means of `.proto` files which define the services and the data
exchanged between server and client.

[Tooling](https://grpc.io/docs/languages/) is available for most popular programming languages to create
client stubs for invoking the services.

The Databroker uses gRPC's default HTTP/2 transport and [protocol buffers](https://developers.google.com/protocol-buffers) for message serialization.
The same `.proto` file can be used to generate server skeleton and client stubs for other transports and serialization formats as well.

HTTP/2 is a binary replacement for HTTP/1.1 used for handling connections, multiplexing (channels) and providing a standardized way to add headers for authorization and TLS for encryption/authentication.
It also supports bi-directional streaming between client and server.

Kuksa.val Databroker implements the following service interfaces:

* [kuksa.val.v1.VAL](../databroker-proto/proto/kuksa/val/v1/val.proto)
* [sdv.databroker.v1.Broker](../databroker-proto/proto/sdv/databroker/v1/broker.proto)
* [sdv.databroker.v1.Collector](../databroker-proto/proto/sdv/databroker/v1/collector.proto)

<p align="right">(<a href="#top">back to top</a>)</p>

## Known Limitations

* Arrays are not supported in conditions as part of queries (i.e. in the WHERE clause).
* Arrays are not supported by the CLI (except for displaying them)

<p align="right">(<a href="#top">back to top</a>)</p>
