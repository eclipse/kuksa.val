## Supported protocols

This file contains an overview what the KUKSA.val Server and databroker each supports. It focuses on gRPC and VISS support and also what feeders are supported.


| Protocol                  | KUKSA.val server | KUKSA.val databroker |
|---------------------------|:----------------:|:--------------------:|
| VISS V1                   |         -        |           -          |
| VISS V2                   |        x/-       |          x/-         |
| gRPC (kuksa)              |         x        |           -          |
| gRPC (kuksa.val.v1)       |         -        |           x          |
| gRPC (sdv.databroker.v1)  |         -        |           x          |

x = supported; x/- = partially supported; - = not supported


### VISSv2 support (websocket transport)

| Feature                       | KUKSA.val server  | KUKSA.val databroker |
|-------------------------------|:-----------------:|:--------------------:|
| Read                          |                   |                      |
|   - Authorized Read           | x<sup>1,2</sup>   |           x          |
|   - Search Read               | -                 |           -          |
|   - History Read              | -                 |           -          |
|   - Static Metadata Read      | -                 |           x          |
|   - Dynamic Metadata Read     | -                 |           -          |
| Update                        |                   |                      |
|   - Authorized Update         | x<sup>1,2</sup>   |           x          |
| Subscribe                     |                   |                      |
|   - Authorized Subscribe      | x<sup>1,2</sup>   |           x          |
|   - Curve Logging Subscribe   | -                 |           -          |
|   - Range Subscribe           | -                 |           -          |
|   - Change Subscribe          | -                 |           -          |
| Unsubscribe                   | x                 |           x          |
| Subscription                  | x                 |           x          |
| Error messages                | x                 |           x          |
| Timestamps                    | x                 |           x          |

x = supported

x<sup>1</sup> Authorization is done using a non-standard standalone call which is incompatible with standards compliant clients.

x<sup>2</sup> Relies on the non-standard `attribute` values which doesn't work with standards compliant clients.

For a more detailed view of the supported JSON-schemas [click here](https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-server/include/VSSRequestJsonSchema.hpp)

### VISSv2 in KUKSA.val server
KUKSA.val server supports the semantics of [VISS v1](https://www.w3.org/TR/vehicle-information-service/) using the new syntax of [VISS v2](https://www.w3.org/TR/viss2-core/). It implements a modified version of VISSv2 which introduces the concept of `attributes` which makes it incompatible with standards compliant VISSv2 clients.
KUKSA.val server doesn't support the VISS V2 security model and there is currently no plan to support it. KUKSA.val server does support authenticated access to VSS resources. For details check [here.](../KUKSA.val_server/jwt.md).

### VISSv2 in KUKSA.val databroker
KUKSA.val databroker aims to provide a standards compliant implementation of VISSv2 (using the websocket transport).

It supports authorization using the access token format specified in [authorization.md](../KUKSA.val_data_broker/authorization.md).

VISSv2 support in databroker is included by building it with the `viss` feature flag.
```shell
$ cargo build --features viss
```

The `enable-viss` flag must be provided at startup in order to enable the VISSv2 websocket interface.

```shell
$ databroker --enable-viss
```

Using kuksa-client, the VISSv2 interface of databroker is available using the `ws` protocol in the uri, i.e.:

```shell
$ kuksa-client ws://127.0.0.1:8090
```

TLS is currently not supported.

### KUKSA.val databroker gRPC API
The VISS Standard is not applicable for gRPC protocols. Here is an overview what the gRPC API in KUKSA.val databroker is capable of:

  * Read: Reading VSS datapoints
    * Reading current or target value for actuators
    * Reading some metadata information from VSS datapoints
  * Write: Writing VSS datapoints
    * Writing sensor values
    * Writing current or target value for actuators
    * Soon: Writing some metadata information from VSS datapoints
  * Subscription: Subscribing VSS datapoints
    * Subscribing sensor values
    * Subscribing current or target value for actuators
