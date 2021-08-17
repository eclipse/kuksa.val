import pyarrow as pa

KUKSA_TO_PYARROW_MAPPING = {
        "int8": pa.int8,
        "int16": pa.int16,
        "int32": pa.int32,
        "int64": pa.int64,
        "uint8": pa.uint8,
        "uint16": pa.uint16,
        "uint32": pa.uint32,
        "uint64": pa.uint64,
        "boolean": pa.bool_,
        "double": pa.float64,
        "float": pa.float32,
        "string": pa.string
}
