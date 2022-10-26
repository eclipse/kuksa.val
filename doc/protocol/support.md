# What do the KUKSA.val Server and data Broker support?
This file contains a clear overview what the KUKSA.val Server and data Broker each supports. It focuses on gRPC and VISS support and also what feeders are supported.

## Supported protocols


| Protocol   |      Server      |  Broker   |
|------------|:----------------:|----------:|
| VISS V1    |      x           |     o     |
| VISS V2    |     x/o          |     o     |
| gRPC       |      x           |     x     |

x = supported; x/o = partly supported; o = not supported


### Server and VISS v1
#### Support for [VISS v1](https://www.w3.org/TR/vehicle-information-service/)
KUKSA.val supports the semantic of VISS v1 but only the new syntax of VISS v2.

Support of semantic for client actions:

| Action        |    Support      |
|-----------    |:---------------:|
| authorize     |       x         |
| getMetadata   |       x         |
| get           |       x         |
| set           |       x         |
| subscribe     |       x         |
| subscription  |       x         |
| unsubscribe   |       x         |
| unsubscribeAll|       o         |

x = supported; o = not supported

### Server and VISS v2

#### Implementation of described components in [VISS v2 Core](https://www.w3.org/TR/viss2-core/):
| component             |Implementation |
|-----------            |:-------------:|
| Access Grant Server   |       o       |
| Access Token Server   |       o       |
| VISS v2 Server        |      x/o      |

x = implemented; x/o = partly implemented; o = not implemented

#### supported methods of [VISS v2 Core](https://www.w3.org/TR/viss2-core/):

| method            |   Support                     |
|-----------        |:-------------:                |
| Read              |            x/o                |
|   - Authorized Read |             x                 |
|   - Search Read   |             o                 |
|   - History Read  |             o                 |
|   - Signal Discovery Read |             o                 |
|   - Dynamic Metadata Read |             o                 |
| Update            |             x                 |
|   - Authorized Update |             x                 |
| Subscribe         |             x                 |
|   - Authorized Subscribe |             x                 |
|   - Curve Logging Subscribe |             o                 |
|   - Range Subscribe |             o                 |
|   - Change Subscribe |             o                 |
| Unsubscribe       |             x                 |
| Subscription      |             x                 |
| Error messages    |             x                 |
| Timestamps        |             x                 |

For a more detailed view of the supported JSON-schemas [click here](https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-Server/include/VSSRequestJsonSchema.hpp)

### gRPC Server
The VISS Standard is not applicable for gRPC protocol. Here is an overview what the gRPC Server is capable of:

| method            |   Support                     |
|-----------        |:-------------:                |
| Read              |            x/o                |
|   - Authorized Read |             x                 |
|   - Search Read   |             o                 |
|   - History Read  |             o                 |
|   - Signal Discovery Read |             o                 |
|   - Dynamic Metadata Read |             o                 |
| Update            |             o                 |
|   - Authorized Update |             o                 |
| Subscribe         |             x                 |
|   - Authorized Subscribe |             x                 |
|   - Curve Logging Subscribe |             o                 |
|   - Range Subscribe |             o                 |
|   - Change Subscribe |             o                 |
| Unsubscribe       |             o                 |
| Subscription      |             x                 |
| Error messages    |             x                 |
| Timestamps        |             x                 |

The gRPC Server uses the logic behind the KUKSA.val websocket Server.

### gRPC Broker
The VISS Standard is not applicable for gRPC protocol. Here is an overview what the gRPC Broker is capable of:

| method            |   Support                     |
|-----------        |:-------------:                |
| Read              |            x/o                |
|   - Authorized Read |             x                 |
|   - Search Read   |             o                 |
|   - History Read  |             o                 |
|   - Signal Discovery Read |             o                 |
|   - Dynamic Metadata Read |             o                 |
| Update            |             x                 |
|   - Authorized Update |             x                 |
| Subscribe         |             x                 |
|   - Authorized Subscribe |             x                 |
|   - Curve Logging Subscribe |             o                 |
|   - Range Subscribe |             x                 |
|   - Change Subscribe |             o                 |
| Unsubscribe       |             x                 |
| Subscription      |             x                 |
| Error messages    |             x                 |
| Timestamps        |             x                 |

## Supported feeders

| Feeder     |   Server (VISS)  | Server (gRPC) |  Broker   |
|------------|:----------------:|:-------------:|----------:|
| CAN        |      x           |  x            | x   |
| SOME/IP    |     planned      |  planned      | x         |
| GPS        |      x           |  x            | x   |

x = supported; o = not supported