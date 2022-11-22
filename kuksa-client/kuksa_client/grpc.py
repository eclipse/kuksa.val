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

import asyncio
import contextlib
import dataclasses
import datetime
import enum
import http
import logging
from typing import Any
from typing import AsyncIterable
from typing import Callable
from typing import Collection
from typing import Dict
from typing import Iterable
from typing import List
from typing import Optional
from pathlib import Path
import uuid

from google.protobuf import json_format
import grpc
from grpc.aio import AioRpcError

from kuksa.val.v1 import types_pb2
from kuksa.val.v1 import val_pb2
from kuksa.val.v1 import val_pb2_grpc


logger = logging.getLogger(__name__)


class DataType(enum.IntEnum):
    UNSPECIFIED     = types_pb2.DATA_TYPE_UNSPECIFIED
    STRING          = types_pb2.DATA_TYPE_STRING
    BOOLEAN         = types_pb2.DATA_TYPE_BOOLEAN
    INT8            = types_pb2.DATA_TYPE_INT8
    INT16           = types_pb2.DATA_TYPE_INT16
    INT32           = types_pb2.DATA_TYPE_INT32
    INT64           = types_pb2.DATA_TYPE_INT64
    UINT8           = types_pb2.DATA_TYPE_UINT8
    UINT16          = types_pb2.DATA_TYPE_UINT16
    UINT32          = types_pb2.DATA_TYPE_UINT32
    UINT64          = types_pb2.DATA_TYPE_UINT64
    FLOAT           = types_pb2.DATA_TYPE_FLOAT
    DOUBLE          = types_pb2.DATA_TYPE_DOUBLE
    TIMESTAMP       = types_pb2.DATA_TYPE_TIMESTAMP
    STRING_ARRAY    = types_pb2.DATA_TYPE_STRING_ARRAY
    BOOLEAN_ARRAY   = types_pb2.DATA_TYPE_BOOLEAN_ARRAY
    INT8_ARRAY      = types_pb2.DATA_TYPE_INT8_ARRAY
    INT16_ARRAY     = types_pb2.DATA_TYPE_INT16_ARRAY
    INT32_ARRAY     = types_pb2.DATA_TYPE_INT32_ARRAY
    INT64_ARRAY     = types_pb2.DATA_TYPE_INT64_ARRAY
    UINT8_ARRAY     = types_pb2.DATA_TYPE_UINT8_ARRAY
    UINT16_ARRAY    = types_pb2.DATA_TYPE_UINT16_ARRAY
    UINT32_ARRAY    = types_pb2.DATA_TYPE_UINT32_ARRAY
    UINT64_ARRAY    = types_pb2.DATA_TYPE_UINT64_ARRAY
    FLOAT_ARRAY     = types_pb2.DATA_TYPE_FLOAT_ARRAY
    DOUBLE_ARRAY    = types_pb2.DATA_TYPE_DOUBLE_ARRAY
    TIMESTAMP_ARRAY = types_pb2.DATA_TYPE_TIMESTAMP_ARRAY


class EntryType(enum.IntEnum):
    UNSPECIFIED = types_pb2.ENTRY_TYPE_UNSPECIFIED
    ATTRIBUTE   = types_pb2.ENTRY_TYPE_ATTRIBUTE
    SENSOR      = types_pb2.ENTRY_TYPE_SENSOR
    ACTUATOR    = types_pb2.ENTRY_TYPE_ACTUATOR


class View(enum.IntEnum):
    UNSPECIFIED   = types_pb2.VIEW_UNSPECIFIED
    CURRENT_VALUE = types_pb2.VIEW_CURRENT_VALUE
    TARGET_VALUE  = types_pb2.VIEW_TARGET_VALUE
    METADATA      = types_pb2.VIEW_METADATA
    FIELDS        = types_pb2.VIEW_FIELDS
    ALL           = types_pb2.VIEW_ALL


class Field(enum.IntEnum):
    UNSPECIFIED                = types_pb2.FIELD_UNSPECIFIED
    PATH                       = types_pb2.FIELD_PATH
    VALUE                      = types_pb2.FIELD_VALUE
    ACTUATOR_TARGET            = types_pb2.FIELD_ACTUATOR_TARGET
    METADATA                   = types_pb2.FIELD_METADATA
    METADATA_DATA_TYPE         = types_pb2.FIELD_METADATA_DATA_TYPE
    METADATA_DESCRIPTION       = types_pb2.FIELD_METADATA_DESCRIPTION
    METADATA_ENTRY_TYPE        = types_pb2.FIELD_METADATA_ENTRY_TYPE
    METADATA_COMMENT           = types_pb2.FIELD_METADATA_COMMENT
    METADATA_DEPRECATION       = types_pb2.FIELD_METADATA_DEPRECATION
    METADATA_UNIT              = types_pb2.FIELD_METADATA_UNIT
    METADATA_VALUE_RESTRICTION = types_pb2.FIELD_METADATA_VALUE_RESTRICTION
    METADATA_ACTUATOR          = types_pb2.FIELD_METADATA_ACTUATOR
    METADATA_SENSOR            = types_pb2.FIELD_METADATA_SENSOR
    METADATA_ATTRIBUTE         = types_pb2.FIELD_METADATA_ATTRIBUTE


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
    def from_grpc_error(cls, error: AioRpcError):
        grpc_code, grpc_reason  = error.code().value
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
        metadata = cls(data_type=DataType(message.data_type), entry_type=EntryType(message.entry_type))
        for field in ('description', 'comment', 'deprecation', 'unit'):
            if message.HasField(field):
                setattr(metadata, field, getattr(message, field))
        if message.HasField('value_restriction'):
            value_restriction = getattr(message.value_restriction, message.value_restriction.WhichOneof('type'))
            metadata.value_restriction = ValueRestriction()
            for field in ('min', 'max'):
                if value_restriction.HasField(field):
                    setattr(metadata.value_restriction, field, getattr(value_restriction, field))
            if value_restriction.allowed_values:
                metadata.value_restriction.allowed_values = list(value_restriction.allowed_values)
        return metadata

    # pylint: disable=too-many-branches
    def to_message(self, value_type: DataType = DataType.UNSPECIFIED) -> types_pb2.Metadata:
        message = types_pb2.Metadata(data_type=self.data_type.value, entry_type=self.entry_type.value)
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
                    message.value_restriction.signed.min = int(self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.signed.max = int(self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.signed.allowed_values.extend(
                        (int(value) for value in self.value_restriction.allowed_values),
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
                    message.value_restriction.unsigned.min = int(self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.unsigned.max = int(self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.unsigned.allowed_values.extend(
                        (int(value) for value in self.value_restriction.allowed_values),
                    )
            elif value_type in (
                DataType.FLOAT,
                DataType.DOUBLE,
                DataType.FLOAT_ARRAY,
                DataType.DOUBLE_ARRAY,
            ):
                if self.value_restriction.min is not None:
                    message.value_restriction.floating_point.min = float(self.value_restriction.min)
                if self.value_restriction.max is not None:
                    message.value_restriction.floating_point.max = float(self.value_restriction.max)
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.floating_point.allowed_values.extend(
                        (float(value) for value in self.value_restriction.allowed_values),
                    )
            elif value_type in (
                DataType.STRING,
                DataType.STRING_ARRAY,
            ):
                if self.value_restriction.allowed_values is not None:
                    message.value_restriction.string.allowed_values.extend(
                        (str(value) for value in self.value_restriction.allowed_values),
                    )
            else:
                raise ValueError(f"Cannot set value_restriction from data type {value_type.name}")
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
        out_dict = {'data_type': self.data_type.name, 'entry_type': self.entry_type.name}
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

    def to_message(self, value_type: DataType) -> types_pb2.Datapoint:
        message = types_pb2.Datapoint()
        def set_array_attr(obj, attr, values):
            array = getattr(obj, attr)
            array.Clear()
            array.values.extend(values)

        def cast_array_values(cast, array):
            for item in array:
                yield cast(item)

        def cast_bool(value):
            if value in ('False', 'false', 'F', 'f'):
                value = 0
            return bool(value)

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
            DataType.BOOLEAN: ('bool', setattr, cast_bool),
            DataType.STRING: ('string', setattr, str),
            DataType.INT8_ARRAY: ('int32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.INT16_ARRAY: ('int32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.INT32_ARRAY: ('int32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.UINT8_ARRAY: ('uint32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.UINT16_ARRAY: ('uint32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.UINT32_ARRAY: ('uint32_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.UINT64_ARRAY: ('uint64_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.INT64_ARRAY: ('int64_array', set_array_attr, lambda array: cast_array_values(int, array)),
            DataType.FLOAT_ARRAY: ('float_array', set_array_attr, lambda array: cast_array_values(float, array)),
            DataType.DOUBLE_ARRAY: ('double_array', set_array_attr, lambda array: cast_array_values(float, array)),
            DataType.BOOLEAN_ARRAY: ('bool_array', set_array_attr, lambda array: cast_array_values(cast_bool, array)),
            DataType.STRING_ARRAY: ('string_array', set_array_attr, lambda array: cast_array_values(str, array)),
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

    value_type: DataType = DataType.UNSPECIFIED  # Useful for serialisation, won't appear directly in serialised object

    @classmethod
    def from_message(cls, message: types_pb2.DataEntry):
        entry_kwargs = {'path': message.path}
        if message.HasField('value'):
            entry_kwargs['value'] = Datapoint.from_message(message.value)
        if message.HasField('actuator_target'):
            entry_kwargs['actuator_target'] = Datapoint.from_message(message.actuator_target)
        if message.HasField('metadata'):
            entry_kwargs['metadata'] = Metadata.from_message(message.metadata)
        return cls(**entry_kwargs)

    def to_message(self) -> types_pb2.DataEntry:
        message = types_pb2.DataEntry(path=self.path)
        if self.value is not None:
            message.value.MergeFrom(self.value.to_message(self.value_type))
        if self.actuator_target is not None:
            message.actuator_target.MergeFrom(self.actuator_target.to_message(self.value_type))
        if self.metadata is not None:
            message.metadata.MergeFrom(self.metadata.to_message(self.value_type))
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
        root_certificates: Optional[Path] = None,
        private_key: Optional[Path] = None,
        certificate_chain: Optional[Path] = None,
        *,
        ensure_startup_connection: bool = True,
    ):
        self.target_host = f'{host}:{port}'
        self.root_certificates = root_certificates
        self.private_key = private_key
        self.certificate_chain = certificate_chain
        self.ensure_startup_connection = ensure_startup_connection
        self.client_stub = None
        self.subscribers = {}

    def _load_creds(self) -> Optional[grpc.ChannelCredentials]:
        if all((self.root_certificates, self.private_key, self.certificate_chain)):
            root_certificates = self.root_certificates.read_bytes()
            private_key = self.private_key.read_bytes()
            certificate_chain = self.certificate_chain.read_bytes()
            return grpc.ssl_channel_credentials(root_certificates, private_key, certificate_chain)
        return None

    def _prepare_get_request(self, entries: Iterable[EntryRequest]) -> val_pb2.GetRequest:
        req = val_pb2.GetRequest(entries=[])
        for entry in entries:
            entry_request = val_pb2.EntryRequest(path=entry.path, view=entry.view.value, fields=[])
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
                paths_with_required_type[update.entry.path] = metadata.data_type if metadata is not None else DataType.UNSPECIFIED
        return paths_with_required_type

    def _prepare_set_request(self, updates: Collection[EntryUpdate], paths_with_required_type: Dict[str, DataType]) -> val_pb2.SetRequest:
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

    def _raise_if_invalid(self, response):
        if response.HasField('error'):
            error = json_format.MessageToDict(response.error, preserving_proto_field_name=True)
        else:
            error = None
        if response.errors:
            errors = [json_format.MessageToDict(err, preserving_proto_field_name=True) for err in response.errors]
        else:
            errors = []
        if error and error['code'] != http.HTTPStatus.OK:
            raise VSSClientError(error, errors)


class VSSClient(BaseVSSClient):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.channel = None
        self.exit_stack = contextlib.AsyncExitStack()

    async def __aenter__(self):
        creds = self._load_creds()
        if creds is not None:
            channel = grpc.aio.secure_channel(self.target_host, creds)
        else:
            channel = grpc.aio.insecure_channel(self.target_host)
        self.channel = await self.exit_stack.enter_async_context(channel)
        self.client_stub = val_pb2_grpc.VALStub(self.channel)
        if self.ensure_startup_connection:
            logger.debug("Connected to server: %s", await self.get_server_info())
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        await self.exit_stack.aclose()
        self.client_stub = None
        self.channel = None

    async def get_current_values(self, paths: Iterable[str]) -> Dict[str, Datapoint]:
        """
        Example:
            current_values = await client.get_current_values([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ])
            speed_value = current_values['Vehicle.Speed'].value
        """
        entries = await self.get(entries=(EntryRequest(path, View.CURRENT_VALUE, (Field.VALUE,)) for path in paths))
        return {entry.path: entry.value for entry in entries}

    async def get_target_values(self, paths: Iterable[str]) -> Dict[str, Datapoint]:
        """
        Example:
            target_values = await client.get_target_values([
                'Vehicle.ADAS.ABS.IsActive',
            ])
            is_abs_to_become_active = target_values['Vehicle.ADAS.ABS.IsActive'].value
        """
        entries = await self.get(entries=(
            EntryRequest(path, View.TARGET_VALUE, (Field.ACTUATOR_TARGET,),
        ) for path in paths))
        return {entry.path: entry.actuator_target for entry in entries}

    async def get_metadata(self, paths: Iterable[str], field: MetadataField = MetadataField.ALL) -> Dict[str, Metadata]:
        """
        Example:
            metadata = await client.get_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ], MetadataField.UNIT)
            speed_unit = metadata['Vehicle.Speed'].unit
        """
        entries = await self.get(entries=(EntryRequest(path, View.METADATA, (Field(field.value),)) for path in paths))
        return {entry.path: entry.metadata for entry in entries}

    async def set_current_values(self, updates: Dict[str, Datapoint]) -> None:
        """
        Example:
            await client.set_current_values({
                'Vehicle.Speed': Datapoint(42),
                'Vehicle.ADAS.ABS.IsActive': Datapoint(False),
            })
        """
        await self.set(updates=[EntryUpdate(DataEntry(path, value=dp), (Field.VALUE,)) for path, dp in updates.items()])

    async def set_target_values(self, updates: Dict[str, Datapoint]) -> None:
        """
        Example:
            await client.set_target_values({'Vehicle.ADAS.ABS.IsActive': Datapoint(True)})
        """
        await self.set(updates=[EntryUpdate(
            DataEntry(path, actuator_target=dp), (Field.ACTUATOR_TARGET,),
        ) for path, dp in updates.items()])

    async def set_metadata(self, updates: Dict[str, Metadata], field: MetadataField = MetadataField.ALL) -> None:
        """
        Example:
            await client.set_metadata({
                'Vehicle.Cabin.Door.Row1.Left.Shade.Position': Metadata(data_type=DataType.FLOAT),
            })
        """
        await self.set(updates=[EntryUpdate(
            DataEntry(path, metadata=md), (Field(field.value),),
        ) for path, md in updates.items()])

    async def subscribe_current_values(
        self, paths: Iterable[str], callback: Callable[[Dict[str, Datapoint]], None],
    ) -> uuid.UUID:
        """
        Example:
            def on_current_values_updated(updates: Dict[str, Datapoint]):
                for path, dp in updates.items():
                    print(f"Current value for {path} is now: {dp.value}")

            subscription_id = await client.subscribe_current_values([
                'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive',
            ], on_current_values_updated)
        """
        return await self.subscribe(
            entries=(SubscribeEntry(path, View.CURRENT_VALUE, (Field.VALUE,)) for path in paths),
            callback=self._subscriber_current_values_callback_wrapper(callback),
        )

    async def subscribe_target_values(
        self, paths: Iterable[str], callback: Callable[[Dict[str, Datapoint]], None],
    ) -> uuid.UUID:
        """
        Example:
            def on_target_values_updated(updates: Dict[str, Datapoint]):
                for path, dp in updates.items():
                    print(f"Target value for {path} is now: {dp.value}")

            subscription_id = await client.subscribe_target_values([
                'Vehicle.ADAS.ABS.IsActive',
            ], on_target_values_updated)
        """
        return await self.subscribe(
            entries=(SubscribeEntry(path, View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)) for path in paths),
            callback=self._subscriber_target_values_callback_wrapper(callback),
        )

    async def subscribe_metadata(
        self, paths: Iterable[str],
        callback: Callable[[Dict[str, Metadata]], None],
        field: MetadataField = MetadataField.ALL,
    ) -> uuid.UUID:
        """
        Example:
            def on_metadata_updated(updates: Dict[str, Metadata]):
                for path, md in updates.items():
                    print(f"Metadata for {path} are now: {md.to_dict()}")

            subscription_id = await client.subscribe_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ], on_metadata_updated)
        """
        return await self.subscribe(
            entries=(SubscribeEntry(path, View.METADATA, (Field(field.value),)) for path in paths),
            callback=self._subscriber_metadata_callback_wrapper(callback),
        )

    async def get(self, *, entries: Iterable[EntryRequest]) -> List[DataEntry]:
        req = self._prepare_get_request(entries)
        try:
            resp = await self.client_stub.Get(req)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        return self._process_get_response(resp)

    async def set(self, *, updates: Collection[EntryUpdate]) -> None:
        paths_with_required_type = self._get_paths_with_required_type(updates)
        paths_without_type = [
            path for path, data_type in paths_with_required_type.items() if data_type is DataType.UNSPECIFIED
        ]
        paths_with_required_type.update(await self.get_value_types(paths_without_type))
        req = self._prepare_set_request(updates, paths_with_required_type)
        try:
            resp = await self.client_stub.Set(req)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        self._process_set_response(resp)

    async def authorize(self, *, token: str) -> str:
        raise NotImplementedError('"authorize" is not yet implemented')

    async def subscribe(self, *,
        entries: Iterable[SubscribeEntry],
        callback: Callable[[Iterable[EntryUpdate]], None],
    ) -> uuid.UUID:
        req = val_pb2.SubscribeRequest()
        for entry in entries:
            entry_request = val_pb2.SubscribeEntry(path=entry.path, view=entry.view.value, fields=[])
            for field in entry.fields:
                entry_request.fields.append(field.value)
            req.entries.append(entry_request)
        logger.debug("%s: %s", type(req).__name__, req)
        resp_stream = self.client_stub.Subscribe(req)
        try:
            # We expect the first SubscribeResponse to be immediately available and to only hold a status
            resp = await resp_stream.__aiter__().__anext__()  # pylint: disable=unnecessary-dunder-call
            logger.debug("%s: %s", type(resp).__name__, resp)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc

        sub_id = uuid.uuid4()
        new_sub_task = asyncio.create_task(self._subscriber_loop(message_stream=resp_stream, callback=callback))
        self.subscribers[sub_id] = new_sub_task
        return sub_id

    async def unsubscribe(self, subscription_id: uuid.UUID):
        try:
            subscriber_task = self.subscribers.pop(subscription_id)
        except KeyError as exc:
            raise ValueError(f"Could not find subscription {str(subscription_id)}") from exc
        subscriber_task.cancel()
        try:
            await subscriber_task
        except asyncio.CancelledError:
            pass

    async def get_server_info(self) -> ServerInfo:
        req = val_pb2.GetServerInfoRequest()
        logger.debug("%s: %s", type(req).__name__, req)
        try:
            resp = await self.client_stub.GetServerInfo(req)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        logger.debug("%s: %s", type(resp).__name__, resp)

        return ServerInfo.from_message(resp)

    async def _subscriber_loop(
        self,
        *,
        message_stream: AsyncIterable[val_pb2.SubscribeResponse],
        callback: Callable[[Iterable[EntryUpdate]], None],
    ):
        async for resp in message_stream:
            callback(EntryUpdate.from_message(update) for update in resp.updates)

    @staticmethod
    def _subscriber_current_values_callback_wrapper(
        callback: Callable[[Dict[str, Datapoint]], None],
    ) -> Callable[[Iterable[EntryUpdate]], None]:
        def wrapper(updates: Iterable[EntryUpdate]) -> None:
            callback({update.entry.path: update.entry.value for update in updates})
        return wrapper

    @staticmethod
    def _subscriber_target_values_callback_wrapper(
        callback: Callable[[Dict[str, Datapoint]], None],
    ) -> Callable[[Iterable[EntryUpdate]], None]:
        def wrapper(updates: Iterable[EntryUpdate]) -> None:
            callback({update.entry.path: update.entry.actuator_target for update in updates})
        return wrapper

    @staticmethod
    def _subscriber_metadata_callback_wrapper(
        callback: Callable[[Dict[str, Metadata]], None],
    ) -> Callable[[Iterable[EntryUpdate]], None]:
        def wrapper(updates: Iterable[EntryUpdate]) -> None:
            callback({update.entry.path: update.entry.metadata for update in updates})
        return wrapper

    async def get_value_types(self, paths: Collection[str]) -> Dict[str, DataType]:
        if paths:
            entry_requests = (EntryRequest(
                path=path, view=View.METADATA, fields=(Field.METADATA_DATA_TYPE,),
            ) for path in paths)
            entries = await self.get(entries=entry_requests)
            return {entry.path: DataType(entry.metadata.data_type) for entry in entries}
        return {}
