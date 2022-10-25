## Why is there a need for the broker?
The KUKSA.val server started as a C++ VISS implementation, that later evolved and gained gRPC support. KUKSA.val databroker was a similar project trying to build a modern rust server for VSS data using a binary efficient gRPC protocol. Currently both projects are being joined, and will support the same GRPC API and Python library so you can exchange feeders and apps.
Check below for the difference in feature set. 

## What is the difference between the KUKSA.val server and broker?

Main differences:

* programming languages: server is written in C++, broker is written in Rust
* VISS support: server does support VISS over Websocket, broker does not
* gRPC API support: server and broker do support a gRPC API (not unified yet but planned)
* Filtering is only supported in the broker (e.g. range filtering, change filtering etc.)
* feeder support: planned is support for all feeders; currently the server supports all feeders and the broker supports the dbc feeder
* security: The KUKSA.val security model with JWT based authentication is currently only supported in server. Support for data broker is on the roadmap

For a detailed overview what the KUKSA.val server and data broker support look [here](protocol/support.md)

## What is the plan for the future? (should be updated freuquently to let the community now what's planned)
Last updated: October 2022 <br>
The server will still be available for those who want to use the VISS.
The broker and the server will support one unified gRPC API.
The broker will be extended to support the kuksa.val server security model.
