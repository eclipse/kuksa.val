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
