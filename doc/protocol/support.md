# What do the KUKSA.val server and data broker support?
This file contains a clear overview what the KUKSA.val server and data broker each supports. It focuses on gRPC and VISS support and also what feeders are supported.

## Supported protocols


| Protocol   |      Server      |  Broker   |
|------------|:----------------:|----------:|
| VISS V1    |      x           |     o     |
| VISS V2    |     x/o          |     o     |
| gRPC       |      x           |     x     |

x = supported; x/o = partly supported; o = not supported


### Server and VISS v1
#### Support for described actions and objects in [VISS V1](https://www.w3.org/TR/vehicle-information-service/)
Support of Request Object - Request Responses:

| Request Object                |      Request Response                                                                                                 |
|:------------:                   |:----------------                                                                                                    |
| authorizeRequest [x]          |authorizeSuccessResponse [x] authorizeErrorResponse [x]                                                                |
| metadataRequest [x]           |metadataSuccessResponse [x] metadataErrorResponse [x]                                                                  |
| getRequest [x]                |getSuccessResponse [x] getErrorResponse [x]                                                                            |
| setRequest [x]                |setSuccessResponse [x] setErrorResponse [x]                                                                            |
| subscribeRequest [x]          |subscribeSuccessResponse [x] subscribeErrorResponse [x] subscriptionNotification [x] subscriptionNotificationError [x] |
| unsubscribeRequest [x]        |unsubscribeSuccessResponse [x] unsubscribeErrorResponse [x]                                                            |
| unsubscribeAllRequest [x]     |unsubscribeAllSuccessResponse [x] unsubscribeAllErrorResponse [x]                                                      |

Support of client actions:

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

#### Implementation of described components in [VISS V2 Core](https://www.w3.org/TR/viss2-core/):
| component             |Implementation |
|-----------            |:-------------:|
| Access Grant Server   |       o       |
| Access Token Server   |       o       |
| VISS v2 Server        |      x/o      |

x = implemnted; x/o = partly implemented; o = not implemented

#### supported methods of [VISS V2 Core](https://www.w3.org/TR/viss2-core/):

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

The gRPC Server uses the logic behind the KUKSA.val websocket server.

### gRPC Broker
The VISS Standard is not applicable for gRPC protocol. Here is an overview what the gRPC Server is capable of:

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

| Feeder     |      Server      |  Broker   |
|------------|:----------------:|----------:|
| CAN        |      x           |  planned  |
| SOME/IP    |     planned      |     x     |
| GPS        |      x           |  planned  |

x = supported; o = not supported