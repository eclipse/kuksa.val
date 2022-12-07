########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
########################################################################

import pyarrow as pa

from kuksa_client.grpc import DataType

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
    "string": pa.string,
}

KUKSA_CLIENT_TO_PYARROW_MAPPING = {
    DataType.STRING: pa.string,
    DataType.BOOLEAN: pa.bool_,
    DataType.INT8: pa.int8,
    DataType.INT16: pa.int16,
    DataType.INT32: pa.int32,
    DataType.INT64: pa.int64,
    DataType.UINT8: pa.uint8,
    DataType.UINT16: pa.uint16,
    DataType.UINT32: pa.uint32,
    DataType.UINT64: pa.uint64,
    DataType.FLOAT: pa.float32,
    DataType.DOUBLE: pa.float64,
    DataType.STRING_ARRAY: pa.StringArray,
    DataType.BOOLEAN_ARRAY: pa.BooleanArray,
    DataType.INT8_ARRAY: pa.Int8Array,
    DataType.INT16_ARRAY: pa.Int16Array,
    DataType.INT32_ARRAY: pa.Int32Array,
    DataType.INT64_ARRAY: pa.Int64Array,
    DataType.UINT8_ARRAY: pa.UInt8Array,
    DataType.UINT16_ARRAY: pa.UInt16Array,
    DataType.UINT32_ARRAY: pa.UInt32Array,
    DataType.UINT64_ARRAY: pa.UInt64Array,
    DataType.FLOAT_ARRAY: pa.FloatingPointArray,
    DataType.DOUBLE_ARRAY: pa.FloatingPointArray,
}
