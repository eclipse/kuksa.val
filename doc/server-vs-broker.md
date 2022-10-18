## Why is there a need for the broker?
The existing VISS implementation is not as leightweight as a gRPC implementation. The existing gRPC server was built on the logic of the Websockt VISS Server. This has let to many workarounds, so the new broker is an approach to provide an user friendly API for In-Vehicle communication over the leightweighted gRPC protocol. The broker is able to resolve query like request. This gives the ability to use the advantages of bidirectional streaming of the gRPC protocol. 

## What is the difference between the KUKSA.val server and broker?

Main differences:

* programming languages: server is written in C++, broker is written in Rust
* VISS support: server does support VISS over Websocket, broker does not
* gRPC API support: server and broker do support a gRPC API (not unified yet but planned)
* Filtering is only support in the broker (e.g. range filtering, change filtering etc.)
* feeder support: planned is support for all feeders; currently the server supports all feeders and the broker supports the dbc feeder

For a detailed overview what the KUKSA.val server and data broker support look [here](protocol/support.md)

## What is the plan for the future? (should be updated freuquently to let the community now what's planned)
Last updated: October 2022 <br>
The server will still be available for those who want to use the VISS.
The broker and the server will support one unified gRPC API.
