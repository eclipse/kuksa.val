## Why is there a need for the KUKSA.val data broker?
The KUKSA.val KUKSA.val server started as a C++ VISS implementation, that later evolved and gained gRPC support. KUKSA.val data broker was a similar project trying to build a modern rust server for VSS data using a binary efficient gRPC protocol. Currently both projects are being joined, and will support the same GRPC API and Python library so you can exchange feeders and apps.
Check below for the difference in feature set. 

## What is the difference between the KUKSA.val server and KUKSA.val data broker?

Main differences:

* programming languages: KUKSA.val server is written in C++, KUKSA.val data broker is written in Rust
* VISS support: KUKSA.val server does support VISS over Websocket, KUKSA.val data broker does not
* gRPC API support: KUKSA.val server and KUKSA.val data broker do support a gRPC API (not unified yet but planned)
* Filtering is only supported in the KUKSA.val data broker (e.g. range filtering, change filtering etc.)
* feeder support: planned is support for all feeders; currently the KUKSA.val server supports all feeders and the KUKSA.val data broker supports the dbc feeder
* security: The KUKSA.val security model with JWT based authentication is currently only supported in KUKSA.val server. Support for data KUKSA.val data broker is on the roadmap

For a detailed overview what the KUKSA.val server and KUKSA.val data broker support look [here](protocol/support.md)

## What is the plan for the future? (should be updated freuquently to let the community now what's planned)
Last updated: October 2022 <br>
The KUKSA.val server will still be available for those who want to use the VISS.
The KUKSA.val data broker and the KUKSA.val server will support one unified gRPC API.
The KUKSA.val data broker will be extended to support the KUKSA.val server security model over JWT Tokens or similar.
