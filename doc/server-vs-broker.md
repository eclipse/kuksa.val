##Why is there a need for the broker?
The existing gRPC server tried to replicate the VISS. For this there are many workarounds which weren't nice. So the new broker is an approach to provide a user friendly API for In-Vehicle communication over the gRPC's. The broker is able to resolve query like request. This gives the ability to use the advantages of bidirectional streaming of the gRPC protocol. 

##What is the difference between the KUKSA.val server and broker?

Main differences:

* programming languages: server is written in C++, broker is written in Rust
* VISS support: server does support VISS over Websocket, broker does not
* gRPC API support: server and broker do support a gRPC API (not unified yet but planned)
* feeder support: planned is support for all feeders; currently the server supports all feeders and the broker supports the dbc feeder

##What is the plan for the future? (should be updated freuquently to let the community now what's planned)
Last updated: October 2022
The server will still be available for those who want to use the VISS.
The broker and the server will support one unified gRPC API.
