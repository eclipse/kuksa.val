## Why is there a need for the KUKSA.val databroker?
The KUKSA.val KUKSA.val server started as a C++ VISS implementation, that later evolved and gained gRPC support. KUKSA.val databroker was a similar project trying to build a modern rust server for VSS data using a binary efficient gRPC protocol. Currently both projects are being joined, and will support the same GRPC API and Python library so you can exchange feeders and apps.
Check below for the difference in feature set.

## What is the difference between the KUKSA.val server and KUKSA.val databroker?

Main differences:

* programming languages: KUKSA.val server is written in C++, KUKSA.val databroker is written in Rust
* VISS support: KUKSA.val server does support VISS over Websocket, KUKSA.val databroker does not
* gRPC API support: KUKSA.val server and KUKSA.val databroker do support a gRPC API (not unified yet but planned)
* Filtering is only supported in the KUKSA.val databroker (e.g. range filtering, change filtering etc.)
* feeder support: planned is support for all feeders; currently the KUKSA.val server supports all feeders and the KUKSA.val databroker supports the dbc feeder
* security: The KUKSA.val security model with JWT based authentication is currently only supported in KUKSA.val server. Support for data KUKSA.val databroker is on the roadmap

For a detailed overview what the KUKSA.val server and KUKSA.val databroker support look [here](protocol/support.md)

## What is the plan for the future? (should be updated frequently to let the community now what's planned)
Last updated: February 2024 <br>

**KUKSA Server is deprecated and will reach End-of-Life 2024-12-31!**
