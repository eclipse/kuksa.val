########################################################################
# Copyright (c) 2022 Robert Bosch GmbH
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

import contextlib
import dataclasses
import datetime
import enum
import http
import logging
import re
from typing import Any
from typing import Collection
from typing import Dict
from typing import Iterable
from typing import Iterator
from typing import List
from typing import Optional
from pathlib import Path

from google.protobuf import json_format
import grpc
from grpc import RpcError

from kuksa.val.v1 import types_pb2
from kuksa.val.v1 import val_pb2
from kuksa.val.v1 import val_pb2_grpc

logger = logging.getLogger(__name__)


class DataType(enum.IntEnum):
    UNSPECIFIED = types_pb2.DATA_TYPE_UNSPECIFIED
    STRING = types_pb2.DATA_TYPE_STRING
    BOOLEAN = types_pb2.DATA_TYPE_BOOLEAN
    INT8 = types_pb2.DATA_TYPE_INT8
    INT16 = types_pb2.DATA_TYPE_INT16
    INT32 = types_pb2.DATA_TYPE_INT32
    INT64 = types_pb2.DATA_TYPE_INT64
    UINT8 = types_pb2.DATA_TYPE_UINT8
    UINT16 = types_pb2.DATA_TYPE_UINT16
    UINT32 = types_pb2.DATA_TYPE_UINT32
    UINT64 = types_pb2.DATA_TYPE_UINT64
    FLOAT = types_pb2.DATA_TYPE_FLOAT
    DOUBLE = types_pb2.DATA_TYPE_DOUBLE
    TIMESTAMP = types_pb2.DATA_TYPE_TIMESTAMP
    STRING_ARRAY = types_pb2.DATA_TYPE_STRING_ARRAY
    BOOLEAN_ARRAY = types_pb2.DATA_TYPE_BOOLEAN_ARRAY
    INT8_ARRAY = types_pb2.DATA_TYPE_INT8_ARRAY
    INT16_ARRAY = types_pb2.DATA_TYPE_INT16_ARRAY
    INT32_ARRAY = types_pb2.DATA_TYPE_INT32_ARRAY
    INT64_ARRAY = types_pb2.DATA_TYPE_INT64_ARRAY
    UINT8_ARRAY = types_pb2.DATA_TYPE_UINT8_ARRAY
    UINT16_ARRAY = types_pb2.DATA_TYPE_UINT16_ARRAY
    UINT32_ARRAY = types_pb2.DATA_TYPE_UINT32_ARRAY
    UINT64_ARRAY = types_pb2.DATA_TYPE_UINT64_ARRAY
    FLOAT_ARRAY = types_pb2.DATA_TYPE_FLOAT_ARRAY
    DOUBLE_ARRAY = types_pb2.DATA_TYPE_DOUBLE_ARRAY
    TIMESTAMP_ARRAY = types_pb2.DATA_TYPE_TIMESTAMP_ARRAY


class EntryType(enum.IntEnum):
    UNSPECIFIED = types_pb2.ENTRY_TYPE_UNSPECIFIED
    ATTRIBUTE = types_pb2.ENTRY_TYPE_ATTRIBUTE
    SENSOR = types_pb2.ENTRY_TYPE_SENSOR
    ACTUATOR = types_pb2.ENTRY_TYPE_ACTUATOR


class View(enum.IntEnum):
    UNSPECIFIED = types_pb2.VIEW_UNSPECIFIED
    CURRENT_VALUE = types_pb2.VIEW_CURRENT_VALUE
    TARGET_VALUE = types_pb2.VIEW_TARGET_VALUE
    METADATA = types_pb2.VIEW_METADATA
    FIELDS = types_pb2.VIEW_FIELDS
    ALL = types_pb2.VIEW_ALL


class Field(enum.IntEnum):
    UNSPECIFIED = types_pb2.FIELD_UNSPECIFIED
    PATH = types_pb2.FIELD_PATH
    VALUE = types_pb2.FIELD_VALUE
    ACTUATOR_TARGET = types_pb2.FIELD_ACTUATOR_TARGET
    METADATA = types_pb2.FIELD_METADATA
    METADATA_DATA_TYPE = types_pb2.FIELD_METADATA_DATA_TYPE
    METADATA_DESCRIPTION = types_pb2.FIELD_METADATA_DESCRIPTION
    METADATA_ENTRY_TYPE = types_pb2.FIELD_METADATA_ENTRY_TYPE
    METADATA_COMMENT = types_pb2.FIELD_METADATA_COMMENT
    METADATA_DEPRECATION = types_pb2.FIELD_METADATA_DEPRECATION
    METADATA_UNIT = types_pb2.FIELD_METADATA_UNIT
    METADATA_VALUE_RESTRICTION = types_pb2.FIELD_METADATA_VALUE_RESTRICTION
    METADATA_ACTUATOR = types_pb2.FIELD_METADATA_ACTUATOR
    METADATA_SENSOR = types_pb2.FIELD_METADATA_SENSOR
    METADATA_ATTRIBUTE = types_pb2.FIELD_METADATA_ATTRIBUTE


class MetadataField(enum.Enum):
    ALL = Field.METADATA
    DATA_TYPE = Field.METADATA_DATA_TYPE
    DESCRIPTION = Field.METADATA_DESCRIPTION
    ENTRY_TYPE = Field.METADATA_ENTRY_TYPE
    COMMENT = Field.METADATA_COMMENT
    DEPRECATION = Field.METADATA_DEPRECATION
    UNIT = Field.METADATA_UNIT
    VALUE_RESTRICTION = Field.METADATA_VALUE_RESTRICTION
    ACTUATOR = Field.METADATA_ACTUATOR
    SENSOR = Field.METADATA_SENSOR
    ATTRIBUTE = Field.METADATA_ATTRIBUTE


class VSSClientError(Exception):
    def __init__(self, error: Dict[str, Any], errors: List[Dict[str, Any]]):
        super().__init__(error, errors)
        self.error = error
        self.errors = errors

    @classmethod
    def from_grpc_error(cls, error: RpcError):
        grpc_code, grpc_reason = error.code().value
        # TODO: Maybe details could hold an actual Error and/or repeated DataEntryError protobuf messages.
        #       This would allow 'code' to be an actual HTTP/VISS status code not a gRPC one.
        return cls(error={'code': grpc_code, 'reason': grpc_reason, 'message': error.details()}, errors=[])

    def to_dict(self) -> Dict[str, Any]:
        return {'error': self.error, 'errors': self.errors}


@dataclasses.dataclass
class ValueRestriction:
    min: Optional[Any] = None
    max: Optional[Any] = None
    allowed_values: Optional[List[Any]] = None


@dataclasses.dataclass
class Metadata:
    data_type: DataType = DataType.UNSPECIFIED
    entry_type: EntryType = EntryType.UNSPECIFIED
    description: Optional[str] = None
    comment: Optional[str] = None
    deprecation: Optional[str] = None
    unit: Optional[str] = None
    value_restriction: Optional[ValueRestriction] = None
    # No support for entry_specific for now.

    @classmethod
    def from_message(cls, message: types_pb2.Metadata):
        metadata = cls(data_type=DataType(message.data_type),
                       entry_type=EntryType(message.entry_type))
        for field in ('description', 'comment', 'deprecation', 'unit'):
            if message.HasField(field):
                setattr(metadata, field, getattr(message, field))
        if message.HasField('value_restriction'):
            value_restriction = getattr(
                message.value_restriction, message.value_restriction.WhichOneof('type'))
            metadata.value_restriction = ValueRestriction()
            for field in ('min', 'max'):
                if value_restriction.HasField(field):
                    setattr(metadata.value_restriction, field,
                            getattr(value_restriction, field))
            if value_restriction.allowed_values:
                metadata.value_restriction.allowed_values = list(
                    value_restriction.allowed_values)
        return metadata

    # pylint: disable=too-many-branches
    def to_message(self, value_type: DataType = DataType.UNSPECIFIED) -> types_pb2.Metadata:
        message = types_pb2.Metadata(
            data_type=self.data_type.value, entry_type=self.entry_type.value)
        for field in ('description', 'comment', 'deprecation', 'unit'):
            field_value = getattr(self, field, None)
            if field_value is not None:
                setattr(message, field, field_value)
        if self.value_restriction is not None:
            if value_type in (
                DataType.INT8,
                DataType.INT16,
                DataType.INT32,
                DataType.INT64,
                DataType.INT8_ARRAY,
                DataType.INT16_ARRAY,
                DataType.INT32_ARRAY,
                DataType.INT64_ARRAY,
            ):
                if self.value_restriction.min is not None:
                    message.value_restriction.signed.min = int(
                        self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.signed.max = int(
                        self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.signed.allowed_values.extend(
                        (int(value)
                         for value in self.value_restriction.allowed_values),
                    )
            elif value_type in (
                DataType.UINT8,
                DataType.UINT16,
                DataType.UINT32,
                DataType.UINT64,
                DataType.UINT8_ARRAY,
                DataType.UINT16_ARRAY,
                DataType.UINT32_ARRAY,
                DataType.UINT64_ARRAY,
            ):
                if self.value_restriction.min is not None:
                    message.value_restriction.unsigned.min = int(
                        self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.unsigned.max = int(
                        self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.unsigned.allowed_values.extend(
                        (int(value)
                         for value in self.value_restriction.allowed_values),
                    )
            elif value_type in (
                DataType.FLOAT,
                DataType.DOUBLE,
                DataType.FLOAT_ARRAY,
                DataType.DOUBLE_ARRAY,
            ):
                if self.value_restriction.min is not None:
                    message.value_restriction.floating_point.min = float(
                        self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.floating_point.max = float(
                        self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.floating_point.allowed_values.extend(
                        (float(value)
                         for value in self.value_restriction.allowed_values),
                    )
            elif value_type in (
                DataType.STRING,
                DataType.STRING_ARRAY,
            ):
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.string.allowed_values.extend(
                        (str(value)
                         for value in self.value_restriction.allowed_values),
                    )
            else:
                raise ValueError(
                    f"Cannot set value_restriction from data type {value_type.name}")
        return message
    # pylint: enable=too-many-branches

    @classmethod
    def from_dict(cls, metadata_dict: Dict[str, Any]):
        data_type = metadata_dict.get('data_type', DataType.UNSPECIFIED)
        if isinstance(data_type, str):
            data_type = getattr(DataType, data_type)
        else:
            data_type = DataType(data_type)
        entry_type = metadata_dict.get('entry_type', EntryType.UNSPECIFIED)
        if isinstance(entry_type, str):
            entry_type = getattr(EntryType, entry_type)
        else:
            entry_type = EntryType(entry_type)
        instance = cls(data_type=data_type, entry_type=entry_type)
        for field in ('description', 'comment', 'deprecation', 'unit'):
            field_value = metadata_dict.get(field, None)
            if field_value is not None:
                setattr(instance, field, str(field_value))
        value_restriction = metadata_dict.get('value_restriction')
        if value_restriction is not None:
            instance.value_restriction = ValueRestriction()
            for field in ('min', 'max', 'allowed_values'):
                field_value = value_restriction.get(field)
                if field_value is not None:
                    setattr(instance.value_restriction, field, field_value)
        return instance

    def to_dict(self) -> Dict[str, Any]:
        out_dict = {'data_type': self.data_type.name,
                    'entry_type': self.entry_type.name}
        for field in ('description', 'comment', 'deprecation', 'unit'):
            field_value = getattr(self, field, None)
            if field_value is not None:
                out_dict[field] = field_value
        if self.value_restriction is not None:
            out_dict['value_restriction'] = {}
            for field in ('min', 'max', 'allowed_values'):
                field_value = getattr(self.value_restriction, field, None)
                if field_value is not None:
                    out_dict['value_restriction'][field] = field_value
        return out_dict


@dataclasses.dataclass
class Datapoint:
    value: Optional[Any] = None
    timestamp: Optional[datetime.datetime] = None

    @classmethod
    def from_message(cls, message: types_pb2.Datapoint):
        return cls(
            value=getattr(message, message.WhichOneof('value')),
            timestamp=message.timestamp.ToDatetime(
                tzinfo=datetime.timezone.utc,
            ) if message.HasField('timestamp') else None,
        )


    def cast_array_values(cast, array):
        """
        Parses array input and cast individual values to wanted type.
        Note that input value to this function is not the same as given if you use kuksa-client command line
        as parts (e.g. surrounding quotes) are removed by shell, and then do_setValue also do some magic.
        """
        array = array.strip('[]')

        # Split the input string into separate values
        # First alternative, not quotes including escaped single or double quote, ends at comma, whitespace or EOL
        # Second group is double quoted string, may contain quoted strings and single quotes inside, ends at non-escaped
        # double quote
        # Third is similar but for single quote
        # Using raw strings with surrounding single/double quotes to minimize need for escapes
        pattern = r'(?:\\"|\\' + \
                  r"'|[^'" + r'",])+|"(?:\\"|[^"])*"|' + \
                  r"'(?:\\'|[^'])*'"
        values = re.findall(pattern, array)
        for item in values:
            # We may in some cases match blanks, that is intended as we want to be able to write arguments like
            # My Way
            # ... without quotes
            if item.strip() == '':
                #skip
                pass
            else:
                yield cast(item)

    def cast_bool(value) -> bool:
        if value in ('False', 'false', 'F', 'f'):
            value = 0
        return bool(value)

    def cast_str(value) -> str:
        """
        Strip based on the following rules.
        - Leading/Trailing blanks are removed
        - If there are single or double quote at both start and end they are removed
        - Finally any quote escapes are removed
        """
        new_val = value.strip()
        if new_val.startswith('"') and new_val.endswith('"'):
            new_val = new_val[1:-1]
        if new_val.startswith("'") and new_val.endswith("'"):
            new_val = new_val[1:-1]
        # Replace escaped quotes with normal quotes
        new_val = new_val.replace('\\\"', '\"')
        new_val = new_val.replace("\\\'", "\'")
        return new_val
            
    def to_message(self, value_type: DataType) -> types_pb2.Datapoint:
        message = types_pb2.Datapoint()

        def set_array_attr(obj, attr, values):
            array = getattr(obj, attr)
            array.Clear()
            array.values.extend(values)


        field, set_field, cast_field = {
            DataType.INT8: ('int32', setattr, int),
            DataType.INT16: ('int32', setattr, int),
            DataType.INT32: ('int32', setattr, int),
            DataType.UINT8: ('uint32', setattr, int),
            DataType.UINT16: ('uint32', setattr, int),
            DataType.UINT32: ('uint32', setattr, int),
            DataType.UINT64: ('uint64', setattr, int),
            DataType.INT64: ('int64', setattr, int),
            DataType.FLOAT: ('float', setattr, float),
            DataType.DOUBLE: ('double', setattr, float),
            DataType.BOOLEAN: ('bool', setattr, Datapoint.cast_bool),
            DataType.STRING: ('string', setattr, Datapoint.cast_str),
            DataType.INT8_ARRAY: ('int32_array', set_array_attr, 
                                  lambda array: Datapoint.cast_array_values(int, array)),
            DataType.INT16_ARRAY: ('int32_array', set_array_attr, 
                                   lambda array: Datapoint.cast_array_values(int, array)),
            DataType.INT32_ARRAY: ('int32_array', set_array_attr, 
                                   lambda array: Datapoint.cast_array_values(int, array)),
            DataType.UINT8_ARRAY: ('uint32_array', set_array_attr, 
                                   lambda array: Datapoint.cast_array_values(int, array)),
            DataType.UINT16_ARRAY: ('uint32_array', set_array_attr, 
                                    lambda array: Datapoint.cast_array_values(int, array)),
            DataType.UINT32_ARRAY: ('uint32_array', set_array_attr, 
                                    lambda array: Datapoint.cast_array_values(int, array)),
            DataType.UINT64_ARRAY: ('uint64_array', set_array_attr, 
                                    lambda array: Datapoint.cast_array_values(int, array)),
            DataType.INT64_ARRAY: ('int64_array', set_array_attr, 
                                   lambda array: Datapoint.cast_array_values(int, array)),
            DataType.FLOAT_ARRAY: ('float_array', set_array_attr, 
                                   lambda array: Datapoint.cast_array_values(float, array)),
            DataType.DOUBLE_ARRAY: ('double_array', set_array_attr, 
                                    lambda array: Datapoint.cast_array_values(float, array)),
            DataType.BOOLEAN_ARRAY: ('bool_array', set_array_attr, 
                                     lambda array: Datapoint.cast_array_values(Datapoint.cast_bool, array)),
            DataType.STRING_ARRAY: ('string_array', set_array_attr, 
                                    lambda array: Datapoint.cast_array_values(Datapoint.cast_str, array)),
        }.get(value_type, (None, None, None))
        if self.value is not None:
            if all((field, set_field, cast_field)):
                set_field(message, field, cast_field(self.value))
            else:
                # Either DataType.TIMESTAMP, DataType.TIMESTAMP_ARRAY or DataType.UNSPECIFIED...
                raise ValueError(
                    f"Cannot determine which field to set with data type {value_type} from value {self.value}",
                )
        if self.timestamp is not None:
            message.timestamp.FromDatetime(self.timestamp)
        return message

    def to_dict(self) -> Dict[str, Any]:
        out_dict = {}
        if self.value is not None:
            out_dict['value'] = self.value
        if self.timestamp is not None:
            out_dict['timestamp'] = self.timestamp.isoformat()
        return out_dict


@dataclasses.dataclass
class DataEntry:
    path: str
    value: Optional[Datapoint] = None
    actuator_target: Optional[Datapoint] = None
    metadata: Optional[Metadata] = None

    # Useful for serialisation, won't appear directly in serialised object
    value_type: DataType = DataType.UNSPECIFIED

    @classmethod
    def from_message(cls, message: types_pb2.DataEntry):
        entry_kwargs = {'path': message.path}
        if message.HasField('value'):
            entry_kwargs['value'] = Datapoint.from_message(message.value)
        if message.HasField('actuator_target'):
            entry_kwargs['actuator_target'] = Datapoint.from_message(
                message.actuator_target)
        if message.HasField('metadata'):
            entry_kwargs['metadata'] = Metadata.from_message(message.metadata)
        return cls(**entry_kwargs)

    def to_message(self) -> types_pb2.DataEntry:
        message = types_pb2.DataEntry(path=self.path)
        if self.value is not None:
            message.value.MergeFrom(self.value.to_message(self.value_type))
        if self.actuator_target is not None:
            message.actuator_target.MergeFrom(
                self.actuator_target.to_message(self.value_type))
        if self.metadata is not None:
            message.metadata.MergeFrom(
                self.metadata.to_message(self.value_type))
        return message

    def to_dict(self) -> Dict[str, Any]:
        out_dict = {'path': self.path}
        if self.value is not None:
            out_dict['value'] = self.value.to_dict()
        if self.actuator_target is not None:
            out_dict['actuator_target'] = self.actuator_target.to_dict()
        if self.metadata is not None:
            out_dict['metadata'] = self.metadata.to_dict()
        return out_dict


@dataclasses.dataclass
class EntryRequest:
    path: str
    view: View
    fields: Iterable[Field]


@dataclasses.dataclass
class EntryUpdate:
    entry: DataEntry
    fields: Iterable[Field]

    @classmethod
    def from_message(cls, message: val_pb2.EntryUpdate):
        return cls(
            entry=DataEntry.from_message(message.entry),
            fields=[Field(field) for field in message.fields],
        )

    def to_message(self) -> val_pb2.EntryUpdate:
        message = val_pb2.EntryUpdate(entry=self.entry.to_message())
        message.fields.extend(field.value for field in self.fields)
        return message

    def to_dict(self) -> Dict[str, Any]:
        return {'entry': self.entry.to_dict(), 'fields': [field.name for field in self.fields]}


@dataclasses.dataclass
class SubscribeEntry:
    path: str
    view: View
    fields: Iterable[Field]


@dataclasses.dataclass
class ServerInfo:
    name: str
    version: str

    @classmethod
    def from_message(cls, message: val_pb2.GetServerInfoResponse):
        return cls(name=message.name, version=message.version)

class BaseVSSClient:
    def __init__(
        self,
        host: str,
        port: int,
        token: Optional[str] = None,
        root_certificates: Optional[Path] = None,
        private_key: Optional[Path] = None,
        certificate_chain: Optional[Path] = None,
        ensure_startup_connection: bool = True,
        connected: bool = False,
        tls_server_name: Optional[str] = None
    ):
    

        self.authorization_header = self.get_authorization_header(token)
        self.target_host = f'{host}:{port}'
        self.root_certificates = root_certificates
        self.private_key = private_key
        self.certificate_chain = certificate_chain
        self.tls_server_name = tls_server_name
        self.ensure_startup_connection = ensure_startup_connection
        self.connected = connected
        self.client_stub = None

    def _load_creds(self) -> Optional[grpc.ChannelCredentials]:
        if self.root_certificates:
            logger.info(f"Using TLS with Root CA from {self.root_certificates}")
            root_certificates = self.root_certificates.read_bytes()
            if self.private_key and self.certificate_chain:
                private_key = self.private_key.read_bytes()
                certificate_chain = self.certificate_chain.read_bytes()
                # As of today there is no option in KUKSA.val Databroker to require client authentication
                logger.info("Using client private key and certificates, mutual TLS supported if supported by server")
                return grpc.ssl_channel_credentials(root_certificates, private_key, certificate_chain)
            else:
                logger.info(f"No client certificates provided, mutual TLS not supported!")
                return grpc.ssl_channel_credentials(root_certificates)
        logger.info(f"No Root CA present, it will not be posible to use a secure connection!")
        return None
        

    def _prepare_get_request(self, entries: Iterable[EntryRequest]) -> val_pb2.GetRequest:
        req = val_pb2.GetRequest(entries=[])
        for entry in entries:
            entry_request = val_pb2.EntryRequest(
                path=entry.path, view=entry.view.value, fields=[])
            for field in entry.fields:
                entry_request.fields.append(field.value)
            req.entries.append(entry_request)
        logger.debug("%s: %s", type(req).__name__, req)
        return req

    def _process_get_response(self, response: val_pb2.GetResponse) -> List[DataEntry]:
        logger.debug("%s: %s", type(response).__name__, response)
        self._raise_if_invalid(response)
        return [DataEntry.from_message(entry) for entry in response.entries]

    def _get_paths_with_required_type(self, updates: Collection[EntryUpdate]) -> Dict[str, DataType]:
        paths_with_required_type = {}
        for update in updates:
            metadata = update.entry.metadata
            # We need a data type in order to set sensor/actuator value or metadata's value restriction
            if (
                Field.ACTUATOR_TARGET in update.fields or
                Field.VALUE in update.fields or
                (metadata is not None and metadata.value_restriction is not None)
            ):
                # If the update holds a new data type, we assume it will be applied before
                # setting the sensor/actuator value.
                paths_with_required_type[update.entry.path] = (
                    metadata.data_type if metadata is not None else DataType.UNSPECIFIED
                )
        return paths_with_required_type

    def _prepare_set_request(
        self, updates: Collection[EntryUpdate], paths_with_required_type: Dict[str, DataType],
    ) -> val_pb2.SetRequest:
        req = val_pb2.SetRequest(updates=[])
        for update in updates:
            value_type = paths_with_required_type.get(update.entry.path)
            if value_type is not None:
                update.entry.value_type = value_type
            req.updates.append(update.to_message())
        logger.debug("%s: %s", type(req).__name__, req)
        return req

    def _process_set_response(self, response: val_pb2.SetResponse) -> None:
        logger.debug("%s: %s", type(response).__name__, response)
        self._raise_if_invalid(response)

    def _prepare_subscribe_request(
        self, entries: Iterable[SubscribeEntry],
    ) -> val_pb2.SubscribeRequest:
        req = val_pb2.SubscribeRequest()
        for entry in entries:
            entry_request = val_pb2.SubscribeEntry(
                path=entry.path, view=entry.view.value, fields=[])
            for field in entry.fields:
                entry_request.fields.append(field.value)
            req.entries.append(entry_request)
        logger.debug("%s: %s", type(req).__name__, req)
        return req

    def _raise_if_invalid(self, response):
        if response.HasField('error'):
            error = json_format.MessageToDict(
                response.error, preserving_proto_field_name=True)
        else:
            error = {}
        if response.errors:
            errors = [json_format.MessageToDict(
                err, preserving_proto_field_name=True) for err in response.errors]
        else:
            errors = []
        if (error and error['code'] is not http.HTTPStatus.OK) or any(
            sub_error['error']['code'] is not http.HTTPStatus.OK for sub_error in errors
        ):
            raise VSSClientError(error, errors)

    def get_authorization_header(self, token: str):
        if token is None:
            return None
        return "Bearer " + token

    def generate_metadata_header(self, metadata: list, header=None) -> list:
        if header == None:
            header = self.authorization_header
        if metadata:
            metadata = dict(metadata)
        else:
            metadata = dict()
        # Overrides old authorization header. This is intentional.
        metadata["authorization"] = header
        return list(metadata.items())


class VSSClient(BaseVSSClient):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.channel = None
        self.exit_stack = contextlib.ExitStack()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.disconnect()

    def check_connected(func):
        def wrapper(self, *args, **kwargs):
            if self.connected:
                return func(self, *args, **kwargs)
            else:
                logger.info(
                    "Disconnected from server! Try connect.")
        return wrapper

    def connect(self, target_host=None):
        creds = self._load_creds()
        if target_host is None:
            target_host = self.target_host
            
        if creds is not None:
            logger.info("Establishing secure channel")
            if self.tls_server_name:
                logger.info(f"Using TLS server name {self.tls_server_name}")
                options = [('grpc.ssl_target_name_override', self.tls_server_name)]
                channel = grpc.secure_channel(target_host, creds, options)
            else:
                logger.debug(f"Not providing explicit TLS server name")
                channel = grpc.secure_channel(target_host, creds)
        else:
            logger.info("Establishing insecure channel")
            channel = grpc.insecure_channel(target_host)
            
        self.channel = self.exit_stack.enter_context(channel)
        self.client_stub = val_pb2_grpc.VALStub(self.channel)
        self.connected = True
        if self.ensure_startup_connection:
            logger.debug("Connected to server: %s", self.get_server_info())

    def disconnect(self):
        self.exit_stack.close()
        self.client_stub = None
        self.channel = None
        self.connected = False

    def get_current_values(self, paths: Iterable[str], **rpc_kwargs) -> Dict[str, Datapoint]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            current_values = client.get_current_values([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ])
            speed_value = current_values['Vehicle.Speed'].value
        """
        entries = self.get(
            entries=(EntryRequest(path, View.CURRENT_VALUE, (Field.VALUE,)) for path in paths), **rpc_kwargs,
        )
        return {entry.path: entry.value for entry in entries}

    def get_target_values(self, paths: Iterable[str], **rpc_kwargs) -> Dict[str, Datapoint]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            target_values = client.get_target_values([
                'Vehicle.ADAS.ABS.IsActive',
            ])
            is_abs_to_become_active = target_values['Vehicle.ADAS.ABS.IsActive'].value
        """
        entries = self.get(entries=(
            EntryRequest(path, View.TARGET_VALUE, (Field.ACTUATOR_TARGET,),
                         ) for path in paths), **rpc_kwargs)
        return {entry.path: entry.actuator_target for entry in entries}

    def get_metadata(
        self, paths: Iterable[str], field: MetadataField = MetadataField.ALL, **rpc_kwargs,
    ) -> Dict[str, Metadata]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            metadata = client.get_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ], MetadataField.UNIT)
            speed_unit = metadata['Vehicle.Speed'].unit
        """
        entries = self.get(
            entries=(EntryRequest(path, View.METADATA, (Field(field.value),)) for path in paths), **rpc_kwargs,
        )
        return {entry.path: entry.metadata for entry in entries}

    def set_current_values(self, updates: Dict[str, Datapoint], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            client.set_current_values({
                'Vehicle.Speed': Datapoint(42),
                'Vehicle.ADAS.ABS.IsActive': Datapoint(False),
            })
        """
        self.set(
            updates=[EntryUpdate(DataEntry(path, value=dp), (Field.VALUE,))
                     for path, dp in updates.items()],
            **rpc_kwargs,
        )

    def set_target_values(self, updates: Dict[str, Datapoint], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            client.set_target_values(
                {'Vehicle.ADAS.ABS.IsActive': Datapoint(True)})
        """
        self.set(updates=[EntryUpdate(
            DataEntry(path, actuator_target=dp), (Field.ACTUATOR_TARGET,),
        ) for path, dp in updates.items()], **rpc_kwargs)

    def set_metadata(
        self, updates: Dict[str, Metadata], field: MetadataField = MetadataField.ALL, **rpc_kwargs,
    ) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            client.set_metadata({
                'Vehicle.Cabin.Door.Row1.Left.Shade.Position': Metadata(data_type=DataType.FLOAT),
            })
        """
        self.set(updates=[EntryUpdate(
            DataEntry(path, metadata=md), (Field(field.value),),
        ) for path, md in updates.items()], **rpc_kwargs)

    def subscribe_current_values(self, paths: Iterable[str], **rpc_kwargs) -> Iterator[Dict[str, Datapoint]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            for updates in client.subscribe_current_values([
                'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive',
            ]):
                for path, dp in updates.items():
                    print(f"Current value for {path} is now: {dp.value}")
        """
        for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.CURRENT_VALUE, (Field.VALUE,))
                     for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.value for update in updates}

    def subscribe_target_values(self, paths: Iterable[str], **rpc_kwargs) -> Iterator[Dict[str, Datapoint]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            async for updates in client.subscribe_target_values([
                'Vehicle.ADAS.ABS.IsActive',
            ]):
                for path, dp in updates.items():
                    print(f"Target value for {path} is now: {dp.value}")
        """
        for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.TARGET_VALUE,
                     (Field.ACTUATOR_TARGET,)) for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.actuator_target for update in updates}

    def subscribe_metadata(
        self, paths: Iterable[str],
        field: MetadataField = MetadataField.ALL,
        **rpc_kwargs,
    ) -> Iterator[Dict[str, Metadata]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            for updates in client.subscribe_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ]):
                for path, md in updates.items():
                    print(f"Metadata for {path} are now: {md.to_dict()}")
        """
        for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.METADATA, (Field(field.value),))
                     for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.metadata for update in updates}

    @check_connected
    def get(self, entries: Iterable[EntryRequest], **rpc_kwargs) -> List[DataEntry]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            rpc_kwargs.get("metadata"))
        req = self._prepare_get_request(entries)
        try:
            resp = self.client_stub.Get(req, **rpc_kwargs)
        except RpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        return self._process_get_response(resp)

    @check_connected
    def set(self, updates: Collection[EntryUpdate], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            rpc_kwargs.get("metadata"))
        paths_with_required_type = self._get_paths_with_required_type(updates)
        paths_without_type = [
            path for path, data_type in paths_with_required_type.items() if data_type is DataType.UNSPECIFIED
        ]
        paths_with_required_type.update(
            self.get_value_types(paths_without_type, **rpc_kwargs))
        req = self._prepare_set_request(updates, paths_with_required_type)
        try:
            resp = self.client_stub.Set(req, **rpc_kwargs)
        except RpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        self._process_set_response(resp)

    # needs to be handled differently
    def subscribe(self, entries: Iterable[SubscribeEntry], **rpc_kwargs) -> Iterator[List[EntryUpdate]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """

        if self.connected:
            rpc_kwargs["metadata"] = self.generate_metadata_header(
                rpc_kwargs.get("metadata"))
            req = self._prepare_subscribe_request(entries)
            resp_stream = self.client_stub.Subscribe(req, **rpc_kwargs)
            try:
                for resp in resp_stream:
                    logger.debug("%s: %s", type(resp).__name__, resp)
                    yield [EntryUpdate.from_message(update) for update in resp.updates]
            except RpcError as exc:
                raise VSSClientError.from_grpc_error(exc) from exc
        else:
            logger.info("Disconnected from server! Try connect.")

    @check_connected
    def authorize(self, token: str, **rpc_kwargs) -> str:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
            token
                string containing the actual token
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            metadata=rpc_kwargs.get("metadata"), header=self.get_authorization_header(token))
        req = val_pb2.GetServerInfoRequest()
        try:
            resp = self.client_stub.GetServerInfo(req, **rpc_kwargs)
        except RpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        logger.debug("%s: %s", type(resp).__name__, resp)
        self.authorization_header = self.get_authorization_header(token)
        return "Authenticated"

    @check_connected
    def get_server_info(self, **rpc_kwargs) -> Optional[ServerInfo]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            metadata=rpc_kwargs.get("metadata"))
        req = val_pb2.GetServerInfoRequest()
        logger.debug("%s: %s", type(req).__name__, req)
        try:
            resp = self.client_stub.GetServerInfo(req, **rpc_kwargs)
            logger.debug("%s: %s", type(resp).__name__, resp)
            return ServerInfo.from_message(resp)
        except RpcError as exc:
            if exc.code() == grpc.StatusCode.UNAUTHENTICATED:
                logger.info("Unauthenticated channel started")
            else:
                raise VSSClientError.from_grpc_error(exc) from exc
        return None
        

    def get_value_types(self, paths: Collection[str], **rpc_kwargs) -> Dict[str, DataType]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        req = self._prepare_get_request(entries)
        """
        if paths:
            entry_requests = (EntryRequest(
                path=path, view=View.METADATA, fields=(
                    Field.METADATA_DATA_TYPE,),
            ) for path in paths)
            entries = self.get(entries=entry_requests, **rpc_kwargs)
            return {entry.path: DataType(entry.metadata.data_type) for entry in entries}
        return {}
