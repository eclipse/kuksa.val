# KUKSA.val gRPC interfaces

## kuksa.val.v1

This interface describes the unified gRPC interface which is split in 2 protobuf definitions:
- [val.proto](../../proto/kuksa/val/v1/val.proto) holding the `VAL` service and associated RPC definitions.
- [types.proto](../../proto/kuksa/val/v1/types.proto) holding the core data types to support VSS needs.

This interface is currently implemented by `kuksa-databroker` and shall soon be implemented by `kuksa-val-server`.

### Error handling (currently)

`Get` and `Set` RPCs rely on `{Get,Set}Response` protobuf message fields to communicate errors i.e.:
```protobuf
repeated DataEntryError errors = 2;
Error error                    = 3;
```

This means the caller of either `Get` or `Set` needs to check those two fields to decide if there is an error or not.
It is easy to miss for someone who would want to implement `Get` or `Set` and could lead to hidden errors.
The nice thing about them though is that they provide detailed errors (one per data entry).

`Subscribe` and `GetServerInfo` don't have such error fields in their respective `*Response` protobuf messages.
Instead, errors if any will result in an exception being raised (e.g. `grpc.aio.AioRpcError` in python's case) and that
already comes without further efforts with protoc-generated code.

In some cases though, `Get` and `Set` might still raise `grpc.aio.AioRpcError` e.g. because of network errors.
This means the client needs to handle both `grpc.aio.AioRpcError` errors and response-message-based errors.

### Error handling (proposal)

In order not to miss errors without sacrificing details in errors' payload, we could:
- Move detailed errors into grpc's `Status.details` which accepts user-defined payloads (see [gRPC error model](https://cloud.google.com/apis/design/errors#error_model))
- Remove `{Get,Set}Response.error(s)?` fields

This is what it would look like:

```protobuf
// google.rpc.Status
message Status {
  int32 code = 1;  // google.rpc.Code
  string message = 2;  // Error summary e.g. "Not allowed to Write Vehicle.Speed" or "Multiple errors encountered"
  repeated DataEntryError details = 3;  // As google.rpc.Status.details accepts `Any` let's use `DataEntryError` details.
}

```

This way clients would only need to care about `grpc.aio.AioRpcError` that might be raised.

## sdv.databroker.v1

This interface describes the gRPC interface one can use to interact solely with `kuksa-databroker`.
It is not implemented by `kuksa-val-server` and is not planned to be in the future.

- [broker.proto](../../kuksa_databroker/proto/sdv/databroker/v1/broker.proto)
- [types.proto](../../kuksa_databroker/proto/sdv/databroker/v1/types.proto)
- [collector.proto](../../kuksa_databroker/proto/sdv/databroker/v1/collector.proto)

## kuksa.proto (deprecated)

[kuksa.proto](../../kuksa-val-server/protos/kuksa.proto)

This interface is only implemented by `kuksa-val-server` which shall soon implement `kuksa.val.v1` instead.
