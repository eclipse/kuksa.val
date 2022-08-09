# Mapping data types

This is how [VSS data types](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/data_types/)
defined by [COVESA VSS](https://covesa.github.io/vehicle_signal_specification/) are mapped to the data types used by
the GRPC interface of the Vehicle Data Broker (VDB).

The GRPC interface uses [protobuf](https://developers.google.com/protocol-buffers/docs/proto3#scalar) to serialize the data and provides metadata to describes the data types

See `enum DataType` in [databroker.proto](../proto/sdv/edge/databroker/v1/databroker.proto) for the data types and `value` in `Datapoint` for for what is actually sent on the wire.

**Note:** Support for timestamps are currently not implemented (except for setting it as the data type).

| VSS data type | VDB metadata (`DataType`) | serialized as `value` |
|---------------|:--------------------------|----------------------:|
| string        | STRING                    |     string (protobuf) |
| boolean       | BOOL                      |     bool   (protobuf) |
| int8          | INT8                      |     sint32 (protobuf) |
| int16         | INT16                     |     sint32 (protobuf) |
| int32         | INT32                     |     sint32 (protobuf) |
| int64         | INT64                     |     sint64 (protobuf) |
| uint8         | UINT8                     |     uint32 (protobuf) |
| uint16        | UINT16                    |     uint32 (protobuf) |
| uint32        | UINT32                    |     uint32 (protobuf) |
| uint64        | UINT64                    |     uint64 (protobuf) |
| float         | FLOAT                     |     float  (protobuf) |
| double        | DOUBLE                    |     double (protobuf) |
| timestamp     | TIMESTAMP                 |                     - |
| string[]      | STRING_ARRAY              |                     - |
| bool[]        | BOOL_ARRAY                |                     - |
| int8[]        | INT8_ARRAY                |                     - |
| int16[]       | INT16_ARRAY               |                     - |
| int32[]       | INT32_ARRAY               |                     - |
| int64[]       | INT64_ARRAY               |                     - |
| uint8[]       | UINT8_ARRAY               |                     - |
| uint16[]      | UINT16_ARRAY              |                     - |
| uint32[]      | UINT32_ARRAY              |                     - |
| uint64[]      | UINT64_ARRAY              |                     - |
| float[]       | FLOAT_ARRAY               |                     - |
| double[]      | DOUBLE_ARRAY              |                     - |
| timestamp[]   | TIMESTAMP_ARRAY           |                     - |