# Documentation

This package holds 3 different APIs depending on your application's requirements:

- `kuksa_client.grpc.aio.VSSClient` provides an asynchronous client that only supports `grpc` to interact with `kuksa_databroker`
  ([check out examples](examples/async-grpc.md)).
- `kuksa_client.grpc.VSSClient` provides a synchronous client that only supports `grpc` to interact with `kuksa_databroker`
  ([check out examples](examples/sync-grpc.md)).
- `kuksa_client.KuksaClientThread` provides a thread-based client that supports both `ws` and `grpc` to interact with either `kuksa-val-server` or `kuksa_databroker`
  ([check out examples](examples/threaded.md)).
