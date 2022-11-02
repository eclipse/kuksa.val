# What do the KUKSA.val Server and databroker support?
This file contains an overview what the KUKSA.val Server and databroker each supports. It focuses on gRPC and VISS support and also what feeders are supported.

## Supported protocols


| Protocol   |      KUKSA.val server      |  KUKSA.val databroker   |
|------------|:-----------------:|----------:|
| VISS V1    |      o           |     o     |
| VISS V2    |     x/o          |     o     |
| gRPC       |      x           |     x     |

x = supported; x/o = partly supported; o = not supported


### Server and VISS 

KUKSA.val server supports the semantic of [VISS v1](https://www.w3.org/TR/vehicle-information-service/) using the new syntax of [VISS v2](https://www.w3.org/TR/viss2-core/).

KUKSA.val only supports the Websocket transport defined in VISS v2.

Support of VISSv2:

### Security model
KUKSA.val does not support the VISS V2 security model and currently we are not planning to support. KUKSA.val server does support authenticated access to VSS ressources. For details check [here.](../KUKSA.val_server/jwt.md).

### Supported VISS (v2) calls

| method                        |   Support                     | Comment                                                                                                           |
|-------------------------------|:-----------------------------:|-------------------------------------------------------------------------------------------------------------------|
| Read                          |            x/o                ||
|   - Authorized Read           |             x                 | Authorization must be performed as standalone call, do not support "in-lining" authorization in read call         |
|   - Search Read               |             o                 ||
|   - History Read              |             o                 ||
|   - Signal Discovery Read     |             o                 ||
|   - Dynamic Metadata Read     |             o                 ||
| Update                        |             x                 ||
|   - Authorized Update         |             x                 | Authorization must be performed as standalone call, do not support "in-lining" authorization in update call       |
| Subscribe                     |             x                 ||
|   - Authorized Subscribe      |             x                 | Authorization must be performed as standalone call, do not support "in-lining" authorization in subscribe call    |
|   - Curve Logging Subscribe   |             o                 ||
|   - Range Subscribe           |             o                 ||
|   - Change Subscribe          |             o                 ||
| Unsubscribe                   |             x                 ||
| Subscription                  |             x                 ||
| Error messages                |             x                 ||
| Timestamps                    |             x                 ||

x = implemented; x/o = partly implemented; o = not implemented

For a more detailed view of the supported JSON-schemas [click here](https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-server/include/VSSRequestJsonSchema.hpp)


### KUKSA.val databroker gRPC API
The VISS Standard is not applicable for gRPC protocol. Here is an overview what the gRPC API in KUKSA.val databroker is capable of:

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

Authorization is currently not supported, but planned.

