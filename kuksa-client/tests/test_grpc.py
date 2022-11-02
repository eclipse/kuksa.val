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
import datetime
import uuid

from google.protobuf import json_format
from google.protobuf import timestamp_pb2
import grpc
import grpc.aio
import pytest

from kuksa.val.v1 import val_pb2
from kuksa.val.v1 import types_pb2

import kuksa_client.grpc
from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import DataType
from kuksa_client.grpc import EntryRequest
from kuksa_client.grpc import EntryType
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import Metadata
from kuksa_client.grpc import MetadataField
from kuksa_client.grpc import ServerInfo
from kuksa_client.grpc import SubscribeEntry
from kuksa_client.grpc import ValueRestriction
from kuksa_client.grpc import View
from kuksa_client.grpc import VSSClient
from kuksa_client.grpc import VSSClientError


def generate_error(code: grpc.StatusCode, details: str):
    def set_error_context(_request, context):
        context.set_code(code)
        context.set_details(details)
    return set_error_context


class TestVSSClientError:
    def test_from_grpc_error(self):
        grpc_error = grpc.aio.AioRpcError(
            code=grpc.StatusCode.INVALID_ARGUMENT,
            initial_metadata=grpc.aio.Metadata(),
            trailing_metadata=grpc.aio.Metadata(),
            details='No datapoints requested',
        )
        client_error = VSSClientError.from_grpc_error(grpc_error)
        expected_client_error = VSSClientError(
            error={'code': 3, 'reason': 'invalid argument', 'message': 'No datapoints requested'}, errors=[],
        )
        assert client_error.error == expected_client_error.error
        assert client_error.errors == expected_client_error.errors

    def test_to_dict(self):
        error = types_pb2.Error(code=404, reason='not_found', message="Does.Not.Exist not found")
        errors = (types_pb2.DataEntryError(path='Does.Not.Exist', error=error),)
        error = json_format.MessageToDict(error, preserving_proto_field_name=True)
        errors = [json_format.MessageToDict(err, preserving_proto_field_name=True) for err in errors]

        assert VSSClientError(error, errors).to_dict() == {
            'error': {'code': 404, 'reason': 'not_found', 'message': "Does.Not.Exist not found"},
            'errors': [{'path': 'Does.Not.Exist', 'error': {
                'code': 404, 'reason': 'not_found', 'message': "Does.Not.Exist not found",
            }}],
        }


class TestMetadata:
    def test_to_message_empty(self):
        assert Metadata().to_message() == types_pb2.Metadata()

    def test_to_message_value_restriction_without_value_type(self):
        with pytest.raises(ValueError) as exc_info:
            assert Metadata(value_restriction=ValueRestriction()).to_message() == types_pb2.Metadata()
        assert exc_info.value.args == ("Cannot set value_restriction from data type UNSPECIFIED",)

    @pytest.mark.parametrize('value_type', (
        DataType.INT8,
        DataType.INT16,
        DataType.INT32,
        DataType.INT64,
        DataType.INT8_ARRAY,
        DataType.INT16_ARRAY,
        DataType.INT32_ARRAY,
        DataType.INT64_ARRAY,
    ))
    @pytest.mark.parametrize('min_value, max_value, allowed_values', [
        (None, None, None),
        (None, None, []),
        (0, None, None),
        (-42, None, None),
        (None, 0, None),
        (None, 42, None),
        (None, None, [-42, 0, 42]),
        (-42, 42, [-42, 0, 42]),
    ])
    def test_to_message_signed_value_restriction(self, value_type, min_value, max_value, allowed_values):
        if (min_value, max_value, allowed_values) == (None, None, None):
            expected_message = types_pb2.Metadata()
        else:
            expected_message = types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                signed=types_pb2.ValueRestrictionInt(min=min_value, max=max_value, allowed_values=allowed_values),
            ))
        assert Metadata(value_restriction=ValueRestriction(
            min=min_value, max=max_value, allowed_values=allowed_values,
        )).to_message(value_type) == expected_message

    @pytest.mark.parametrize('value_type', (
        DataType.UINT8,
        DataType.UINT16,
        DataType.UINT32,
        DataType.UINT64,
        DataType.UINT8_ARRAY,
        DataType.UINT16_ARRAY,
        DataType.UINT32_ARRAY,
        DataType.UINT64_ARRAY,
    ))
    @pytest.mark.parametrize('min_value, max_value, allowed_values', [
        (None, None, None),
        (None, None, []),
        (0, None, None),
        (None, 0, None),
        (None, 42, None),
        (None, None, [0, 12, 42]),
        (0, 42, [0, 12, 42]),
    ])
    def test_to_message_unsigned_value_restriction(self, value_type, min_value, max_value, allowed_values):
        if (min_value, max_value, allowed_values) == (None, None, None):
            expected_message = types_pb2.Metadata()
        else:
            expected_message = types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                unsigned=types_pb2.ValueRestrictionUint(min=min_value, max=max_value, allowed_values=allowed_values),
            ))
        assert Metadata(value_restriction=ValueRestriction(
            min=min_value, max=max_value, allowed_values=allowed_values,
        )).to_message(value_type) == expected_message

    @pytest.mark.parametrize('value_type', (
        DataType.FLOAT,
        DataType.DOUBLE,
        DataType.FLOAT_ARRAY,
        DataType.DOUBLE_ARRAY,
    ))
    @pytest.mark.parametrize('min_value, max_value, allowed_values', [
        (None, None, None),
        (None, None, []),
        (0, None, None),
        (0.5, None, None),
        (None, 0, None),
        (None, 41.5, None),
        (None, None, [0.5, 12., 41.5]),
        (0.5, 41.5, [0.5, 12., 41.5]),
    ])
    def test_to_message_float_value_restriction(self, value_type, min_value, max_value, allowed_values):
        if (min_value, max_value, allowed_values) == (None, None, None):
            expected_message = types_pb2.Metadata()
        else:
            expected_message = types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                floating_point=types_pb2.ValueRestrictionFloat(min=min_value, max=max_value, allowed_values=allowed_values),
            ))
        assert Metadata(value_restriction=ValueRestriction(
            min=min_value, max=max_value, allowed_values=allowed_values,
        )).to_message(value_type) == expected_message

    @pytest.mark.parametrize('value_type', (DataType.STRING, DataType.STRING_ARRAY))
    @pytest.mark.parametrize('allowed_values', [None, [], ['Hello', 'world']])
    def test_to_message_string_value_restriction(self, value_type, allowed_values):
        if allowed_values is None:
            expected_message = types_pb2.Metadata()
        else:
            expected_message = types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                string=types_pb2.ValueRestrictionString(allowed_values=allowed_values),
            ))
        assert Metadata(value_restriction=ValueRestriction(
            allowed_values=allowed_values,
        )).to_message(value_type) == expected_message

    @pytest.mark.parametrize('metadata_dict, init_kwargs', [
        ({}, {}),
        ({'entry_type': 1}, {'entry_type': EntryType.ATTRIBUTE}),
        ({'entry_type': EntryType.ATTRIBUTE}, {'entry_type': EntryType.ATTRIBUTE}),
        ({'entry_type': 'ATTRIBUTE'}, {'entry_type': EntryType.ATTRIBUTE}),
        ({'data_type': 1}, {'data_type': DataType.STRING}),
        ({'data_type': DataType.STRING}, {'data_type': DataType.STRING}),
        ({'data_type': 'STRING'}, {'data_type': DataType.STRING}),
        ({'description': 'a real description'}, {'description': 'a real description'}),
        ({'comment': 'a real comment'}, {'comment': 'a real comment'}),
        ({'deprecation': 'a real deprecation'}, {'deprecation': 'a real deprecation'}),
        ({'unit': 'a real unit'}, {'unit': 'a real unit'}),
        ({'value_restriction': {}}, {'value_restriction': ValueRestriction()}),
        ({'value_restriction': {'min': 12}}, {'value_restriction': ValueRestriction(min=12)}),
        ({'value_restriction': {'max': 42}}, {'value_restriction': ValueRestriction(max=42)}),
        ({'value_restriction': {'allowed_values': [-42, 42]}}, {
            'value_restriction': ValueRestriction(allowed_values=[-42, 42]),
        }),
        ({'value_restriction': {'min': -42, 'max': 42, 'allowed_values': [-42, 42]}}, {
            'value_restriction': ValueRestriction(min=-42, max=42, allowed_values=[-42, 42]),
        }),
    ])
    def test_from_dict(self, metadata_dict, init_kwargs):
        assert Metadata.from_dict(metadata_dict) == Metadata(**init_kwargs)

    @pytest.mark.parametrize('init_kwargs, metadata_dict', [
        ({}, {'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED'}),
        ({'entry_type': EntryType.ATTRIBUTE}, {'data_type': 'UNSPECIFIED', 'entry_type': 'ATTRIBUTE'}),
        ({'data_type': DataType.STRING}, {'data_type': 'STRING', 'entry_type': 'UNSPECIFIED'}),
        ({'description': 'a real description'}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED', 'description': 'a real description',
        }),
        ({'comment': 'a real comment'}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED', 'comment': 'a real comment',
        }),
        ({'deprecation': 'a real deprecation'}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED', 'deprecation': 'a real deprecation',
        }),
        ({'unit': 'a real unit'}, {'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED', 'unit': 'a real unit'}),
        ({'value_restriction': ValueRestriction()}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED', 'value_restriction': {},
        }),
        ({'value_restriction': ValueRestriction(min=12)}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED',
            'value_restriction': {'min': 12},
        }),
        ({'value_restriction': ValueRestriction(max=42)}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED',
            'value_restriction': {'max': 42},
        }),
        ({'value_restriction': ValueRestriction(allowed_values=[-42, 42])}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED',
            'value_restriction': {'allowed_values': [-42, 42]},
        }),
        ({'value_restriction': ValueRestriction(min=-42, max=42, allowed_values=[-42, 42])}, {
            'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED',
            'value_restriction': {'min': -42, 'max': 42, 'allowed_values': [-42, 42]},
        }),
    ])
    def test_to_dict(self, init_kwargs, metadata_dict):
        assert Metadata(**init_kwargs).to_dict() == metadata_dict


class TestDatapoint:
    @pytest.mark.parametrize('value_type, init_args, message', [
        (DataType.BOOLEAN, (None,), types_pb2.Datapoint()),
        (DataType.BOOLEAN, ('False',), types_pb2.Datapoint(bool=False)),
        (DataType.BOOLEAN, ('false',), types_pb2.Datapoint(bool=False)),
        (DataType.BOOLEAN, ('F',), types_pb2.Datapoint(bool=False)),
        (DataType.BOOLEAN, ('f',), types_pb2.Datapoint(bool=False)),
        (DataType.BOOLEAN, (True, datetime.datetime(2022, 11, 16, tzinfo=datetime.timezone.utc)), types_pb2.Datapoint(
            bool=True, timestamp=timestamp_pb2.Timestamp(seconds=1668556800),
        )),
        (DataType.INT8_ARRAY, ([-128, 127],), types_pb2.Datapoint(int32_array=types_pb2.Int32Array(values=[-128, 127]))),
    ])
    def test_to_message(self, value_type, init_args, message):
        assert Datapoint(*init_args).to_message(value_type) == message

    @pytest.mark.parametrize('value_type', [DataType.UNSPECIFIED, DataType.TIMESTAMP, DataType.TIMESTAMP_ARRAY])
    def test_to_message_unsupported_value_type(self, value_type):
        with pytest.raises(ValueError) as exc_info:
            Datapoint(42).to_message(value_type)
        assert exc_info.value.args[0].startswith('Cannot determine which field to set with data type')

    @pytest.mark.parametrize('init_kwargs, datapoint_dict', [
        ({}, {}),
        ({'value': 42}, {'value': 42}),
        ({'timestamp': datetime.datetime(2022, 11, 16, tzinfo=datetime.timezone.utc)}, {
            'timestamp': '2022-11-16T00:00:00+00:00',
        }),
        ({'value': 42, 'timestamp': datetime.datetime(2022, 11, 16, tzinfo=datetime.timezone.utc)}, {
            'value': 42, 'timestamp': '2022-11-16T00:00:00+00:00',
        }),
    ])
    def test_to_dict(self, init_kwargs, datapoint_dict):
        assert Datapoint(**init_kwargs).to_dict() == datapoint_dict


class TestDataEntry:
    @pytest.mark.parametrize('value, actuator_target, metadata, entry_dict', [
        (None, None, None, {}),
        (Datapoint(42), None, None, {'value': {'value': 42}}),
        (None, Datapoint(42), None, {'actuator_target': {'value': 42}}),
        (None, None, Metadata(), {'metadata': {'data_type': 'UNSPECIFIED', 'entry_type': 'UNSPECIFIED'}}),
    ])
    def test_to_dict(self, value, actuator_target, metadata, entry_dict):
        entry_dict.update({'path': 'Test.Path'})
        assert DataEntry('Test.Path', value, actuator_target, metadata).to_dict() == entry_dict


class TestEntryUpdate:
    @pytest.mark.parametrize('entry, fields, update_dict', [
        (DataEntry(path='Test.Path'), (), {'entry': {'path': 'Test.Path'}, 'fields': []}),
        (DataEntry(path='Test.Path', value=Datapoint(42)), (Field.VALUE,), {
            'entry': {'path': 'Test.Path', 'value': {'value': 42}}, 'fields': ['VALUE'],
        }),
    ])
    def test_to_dict(self, entry, fields, update_dict):
        assert EntryUpdate(entry, fields).to_dict() == update_dict


@pytest.mark.asyncio
class TestVSSClient:
    @pytest.mark.usefixtures('secure_val_server')
    async def test_secure_connection(self, unused_tcp_port, resources_path, val_servicer):
        val_servicer.GetServerInfo.return_value = val_pb2.GetServerInfoResponse(name='test_server', version='1.2.3')
        async with VSSClient('localhost', unused_tcp_port,
            root_certificates=resources_path / 'test-ca.pem',
            private_key=resources_path / 'test-client.key',
            certificate_chain=resources_path / 'test-client.pem',
        ):
            assert val_servicer.GetServerInfo.call_count == 1

    async def test_get_current_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'get', return_value=[
            DataEntry('Vehicle.Speed', value=Datapoint(
                42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc),
            )),
            DataEntry('Vehicle.ADAS.ABS.IsActive', value=Datapoint(True)),
            DataEntry('Vehicle.Chassis.Height', value=Datapoint(666)),
        ])
        assert await client.get_current_values([
            'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive', 'Vehicle.Chassis.Height',
        ]) == {
            'Vehicle.Speed': Datapoint(
                42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc),
            ),
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True),
            'Vehicle.Chassis.Height': Datapoint(666),
        }
        assert list(client.get.call_args_list[0][1]['entries']) == [
            EntryRequest('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
            EntryRequest('Vehicle.ADAS.ABS.IsActive', View.CURRENT_VALUE, (Field.VALUE,)),
            EntryRequest('Vehicle.Chassis.Height', View.CURRENT_VALUE, (Field.VALUE,)),
        ]

    async def test_get_target_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'get', return_value=[
            DataEntry('Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(
                True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc),
            )),
            DataEntry('Vehicle.Chassis.SteeringWheel.Tilt', actuator_target=Datapoint(42)),
        ])
        assert await client.get_target_values([
            'Vehicle.ADAS.ABS.IsActive', 'Vehicle.Chassis.SteeringWheel.Tilt',
        ]) == {
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc)),
            'Vehicle.Chassis.SteeringWheel.Tilt': Datapoint(42),
        }
        assert list(client.get.call_args_list[0][1]['entries']) == [
            EntryRequest('Vehicle.ADAS.ABS.IsActive', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
            EntryRequest('Vehicle.Chassis.SteeringWheel.Tilt', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
        ]

    async def test_get_metadata(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'get', return_value=[
            DataEntry('Vehicle.Speed', metadata=Metadata(entry_type=EntryType.SENSOR)),
            DataEntry('Vehicle.ADAS.ABS.IsActive', metadata=Metadata(entry_type=EntryType.ACTUATOR)),
            DataEntry('Vehicle.Chassis.Height', metadata=Metadata(entry_type=EntryType.ATTRIBUTE)),
        ])
        assert await client.get_metadata([
            'Vehicle.Speed',
            'Vehicle.ADAS.ABS.IsActive',
            'Vehicle.Chassis.Height',
        ], field=MetadataField.ENTRY_TYPE) == {
            'Vehicle.Speed': Metadata(entry_type=EntryType.SENSOR),
            'Vehicle.ADAS.ABS.IsActive': Metadata(entry_type=EntryType.ACTUATOR),
            'Vehicle.Chassis.Height': Metadata(entry_type=EntryType.ATTRIBUTE),
        }
        assert list(client.get.call_args_list[0][1]['entries']) == [
            EntryRequest('Vehicle.Speed', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
            EntryRequest('Vehicle.ADAS.ABS.IsActive', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
            EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
        ]

    async def test_set_current_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'set')
        await client.set_current_values({
            'Vehicle.Speed': Datapoint(42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc)),
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True),
            'Vehicle.Chassis.Height': Datapoint(666),
        })
        assert list(client.set.call_args_list[0][1]['updates']) == [
            EntryUpdate(DataEntry('Vehicle.Speed', value=Datapoint(
                42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc),
            )), (Field.VALUE,)),
            EntryUpdate(DataEntry('Vehicle.ADAS.ABS.IsActive', value=Datapoint(True)), (Field.VALUE,)),
            EntryUpdate(DataEntry('Vehicle.Chassis.Height', value=Datapoint(666)), (Field.VALUE,)),
        ]

    async def test_set_target_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'set')
        await client.set_target_values({
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc)),
            'Vehicle.Chassis.SteeringWheel.Tilt': Datapoint(42),
        })
        assert list(client.set.call_args_list[0][1]['updates']) == [
            EntryUpdate(DataEntry('Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(
                True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc),
            )), (Field.ACTUATOR_TARGET,)),
            EntryUpdate(DataEntry('Vehicle.Chassis.SteeringWheel.Tilt', actuator_target=Datapoint(42)), (Field.ACTUATOR_TARGET,)),
        ]

    async def test_set_metadata(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'set')
        await client.set_metadata({
            'Vehicle.Speed': Metadata(entry_type=EntryType.SENSOR),
            'Vehicle.ADAS.ABS.IsActive': Metadata(entry_type=EntryType.ACTUATOR),
            'Vehicle.Chassis.Height': Metadata(entry_type=EntryType.ATTRIBUTE),
        }, field=MetadataField.ENTRY_TYPE)
        assert list(client.set.call_args_list[0][1]['updates']) == [
            EntryUpdate(DataEntry(
                'Vehicle.Speed', metadata=Metadata(entry_type=EntryType.SENSOR),
            ), (Field.METADATA_ENTRY_TYPE,)),
            EntryUpdate(DataEntry(
                'Vehicle.ADAS.ABS.IsActive', metadata=Metadata(entry_type=EntryType.ACTUATOR),
            ), (Field.METADATA_ENTRY_TYPE,)),
            EntryUpdate(DataEntry(
                'Vehicle.Chassis.Height', metadata=Metadata(entry_type=EntryType.ATTRIBUTE),
            ), (Field.METADATA_ENTRY_TYPE,)),
        ]

    async def test_subscribe_current_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'subscribe')
        received_updates = {}
        def callback(updates):
            received_updates.update(updates)

        await client.subscribe_current_values([
            'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive', 'Vehicle.Chassis.Height',
        ], callback)

        assert list(client.subscribe.call_args_list[0][1]['entries']) == [
            SubscribeEntry('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
            SubscribeEntry('Vehicle.ADAS.ABS.IsActive', View.CURRENT_VALUE, (Field.VALUE,)),
            SubscribeEntry('Vehicle.Chassis.Height', View.CURRENT_VALUE, (Field.VALUE,)),
        ]
        # Simulate subscription updates
        client.subscribe.call_args_list[0][1]['callback']([
            EntryUpdate(DataEntry('Vehicle.Speed', value=Datapoint(
                42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc),
            )), (Field.VALUE,)),
            EntryUpdate(DataEntry('Vehicle.ADAS.ABS.IsActive', value=Datapoint(True)), (Field.VALUE,)),
            EntryUpdate(DataEntry('Vehicle.Chassis.Height', value=Datapoint(666)), (Field.VALUE,)),
        ])
        assert received_updates == {
            'Vehicle.Speed': Datapoint(42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc)),
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True),
            'Vehicle.Chassis.Height': Datapoint(666),
        }

    async def test_subscribe_target_values(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'subscribe')
        received_updates = {}
        def callback(updates):
            received_updates.update(updates)

        await client.subscribe_target_values([
            'Vehicle.ADAS.ABS.IsActive', 'Vehicle.Chassis.SteeringWheel.Tilt',
        ], callback)

        assert list(client.subscribe.call_args_list[0][1]['entries']) == [
            SubscribeEntry('Vehicle.ADAS.ABS.IsActive', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
            SubscribeEntry('Vehicle.Chassis.SteeringWheel.Tilt', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
        ]
        # Simulate subscription updates
        client.subscribe.call_args_list[0][1]['callback']([
            EntryUpdate(DataEntry('Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(
                True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc),
            )), (Field.ACTUATOR_TARGET,)),
            EntryUpdate(DataEntry('Vehicle.Chassis.SteeringWheel.Tilt', actuator_target=Datapoint(42)), (Field.ACTUATOR_TARGET,)),
        ])
        assert received_updates == {
            'Vehicle.ADAS.ABS.IsActive': Datapoint(True, datetime.datetime(2022, 11, 7, tzinfo=datetime.timezone.utc)),
            'Vehicle.Chassis.SteeringWheel.Tilt': Datapoint(42),
        }

    async def test_subscribe_metadata(self, mocker, unused_tcp_port):
        client = VSSClient('127.0.0.1', unused_tcp_port)
        mocker.patch.object(client, 'subscribe')
        received_updates = {}
        def callback(updates):
            received_updates.update(updates)

        await client.subscribe_metadata([
            'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive', 'Vehicle.Chassis.Height',
        ], callback, field=MetadataField.ENTRY_TYPE)

        assert list(client.subscribe.call_args_list[0][1]['entries']) == [
            SubscribeEntry('Vehicle.Speed', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
            SubscribeEntry('Vehicle.ADAS.ABS.IsActive', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
            SubscribeEntry('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
        ]
        # Simulate subscription updates
        client.subscribe.call_args_list[0][1]['callback']([
            EntryUpdate(DataEntry(
                'Vehicle.Speed', metadata=Metadata(entry_type=EntryType.SENSOR),
            ), (Field.METADATA_ENTRY_TYPE,)),
            EntryUpdate(DataEntry(
                'Vehicle.ADAS.ABS.IsActive', metadata=Metadata(entry_type=EntryType.ACTUATOR),
            ), (Field.METADATA_ENTRY_TYPE,)),
            EntryUpdate(DataEntry(
                'Vehicle.Chassis.Height', metadata=Metadata(entry_type=EntryType.ATTRIBUTE),
            ), (Field.METADATA_ENTRY_TYPE,)),
        ])
        assert received_updates == {
            'Vehicle.Speed': Metadata(entry_type=EntryType.SENSOR),
            'Vehicle.ADAS.ABS.IsActive': Metadata(entry_type=EntryType.ACTUATOR),
            'Vehicle.Chassis.Height': Metadata(entry_type=EntryType.ATTRIBUTE),
        }

    @pytest.mark.usefixtures('val_server')
    async def test_get_some_entries(self, unused_tcp_port, val_servicer):
        val_servicer.Get.return_value = val_pb2.GetResponse(entries=[
            types_pb2.DataEntry(
                path='Vehicle.Speed',
                value=types_pb2.Datapoint(
                    timestamp=timestamp_pb2.Timestamp(seconds=1667837915, nanos=247307674),
                    float=42.0,
                ),
            ),
            types_pb2.DataEntry(path='Vehicle.ADAS.ABS.IsActive', actuator_target=types_pb2.Datapoint(bool=True)),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height',
                metadata=types_pb2.Metadata(
                    data_type=types_pb2.DATA_TYPE_UINT16,
                    entry_type=types_pb2.ENTRY_TYPE_ATTRIBUTE,
                    description="Overall vehicle height, in mm.",
                    comment="No comment.",
                    deprecation="V2.1 moved to Vehicle.Height",
                    unit="mm",
                ),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height', metadata=types_pb2.Metadata(data_type=types_pb2.DATA_TYPE_UINT16),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height', metadata=types_pb2.Metadata(entry_type=types_pb2.ENTRY_TYPE_ATTRIBUTE),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height',
                metadata=types_pb2.Metadata(description="Overall vehicle height, in mm."),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height', metadata=types_pb2.Metadata(comment="No comment."),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Chassis.Height', metadata=types_pb2.Metadata(deprecation="V2.1 moved to Vehicle.Height"),
            ),
            types_pb2.DataEntry(path='Vehicle.Chassis.Height', metadata=types_pb2.Metadata(unit="mm")),
            types_pb2.DataEntry(
                path='Vehicle.CurrentLocation.Heading',
                metadata=types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                    floating_point=types_pb2.ValueRestrictionFloat(min=0, max=360),
                )),
            ),
            types_pb2.DataEntry(
                path='Dummy.With.Allowed.Values',
                metadata=types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                    signed=types_pb2.ValueRestrictionInt(allowed_values=[12, 42, 666]),
                )),
            ),
        ])
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            entries = await client.get(entries=(entry for entry in (  # generator is intentional as get accepts Iterable
                EntryRequest('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
                EntryRequest('Vehicle.ADAS.ABS.IsActive', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_DATA_TYPE,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_DESCRIPTION,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_ENTRY_TYPE,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_COMMENT,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_DEPRECATION,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_UNIT,)),
                EntryRequest('Vehicle.CurrentLocation.Heading', View.METADATA, (Field.METADATA_VALUE_RESTRICTION,)),
                EntryRequest('Dummy.With.Allowed.Values', View.METADATA, (Field.METADATA_VALUE_RESTRICTION,)),
            )))
            assert entries == [
                DataEntry('Vehicle.Speed', value=Datapoint(
                    42.0, datetime.datetime(2022, 11, 7, 16, 18, 35, 247307, tzinfo=datetime.timezone.utc),
                )),
                DataEntry('Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(True)),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(
                    data_type=DataType.UINT16,
                    entry_type=EntryType.ATTRIBUTE,
                    description="Overall vehicle height, in mm.",
                    comment="No comment.",
                    deprecation="V2.1 moved to Vehicle.Height",
                    unit="mm",
                )),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(data_type=DataType.UINT16)),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(entry_type=EntryType.ATTRIBUTE)),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(description="Overall vehicle height, in mm.")),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(comment="No comment.")),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(deprecation="V2.1 moved to Vehicle.Height")),
                DataEntry('Vehicle.Chassis.Height', metadata=Metadata(unit="mm")),
                DataEntry('Vehicle.CurrentLocation.Heading', metadata=Metadata(
                    value_restriction=ValueRestriction(min=0, max=360),
                )),
                DataEntry('Dummy.With.Allowed.Values', metadata=Metadata(
                    value_restriction=ValueRestriction(allowed_values=[12, 42, 666]),
                )),
            ]
            assert val_servicer.Get.call_args[0][0].entries == val_pb2.GetRequest(entries=(
                val_pb2.EntryRequest(
                    path='Vehicle.Speed', view=types_pb2.VIEW_CURRENT_VALUE, fields=(types_pb2.FIELD_VALUE,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.ADAS.ABS.IsActive',
                  view=types_pb2.VIEW_TARGET_VALUE,
                  fields=(types_pb2.FIELD_ACTUATOR_TARGET,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_DATA_TYPE,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_DESCRIPTION,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_ENTRY_TYPE,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_COMMENT,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_DEPRECATION,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.Chassis.Height',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_UNIT,),
                ),
                val_pb2.EntryRequest(
                  path='Vehicle.CurrentLocation.Heading',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_VALUE_RESTRICTION,),
                ),
                val_pb2.EntryRequest(
                  path='Dummy.With.Allowed.Values',
                  view=types_pb2.VIEW_METADATA,
                  fields=(types_pb2.FIELD_METADATA_VALUE_RESTRICTION,),
                ),
            )).entries

    @pytest.mark.usefixtures('val_server')
    async def test_get_no_entries_requested(self, unused_tcp_port, val_servicer):
        val_servicer.Get.side_effect = generate_error(grpc.StatusCode.INVALID_ARGUMENT, 'No datapoints requested')
        async with kuksa_client.grpc.VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(kuksa_client.grpc.VSSClientError) as exc_info:
                await client.get(entries=[])

            assert exc_info.value.args == kuksa_client.grpc.VSSClientError(error={
                'code': grpc.StatusCode.INVALID_ARGUMENT.value[0],
                'reason': grpc.StatusCode.INVALID_ARGUMENT.value[1],
                'message': 'No datapoints requested',
            }, errors=[]).args
            assert val_servicer.Get.call_args[0][0] == val_pb2.GetRequest()

    @pytest.mark.usefixtures('val_server')
    async def test_get_unset_entries(self, unused_tcp_port, val_servicer):
        val_servicer.Get.return_value = val_pb2.GetResponse(entries=[
            types_pb2.DataEntry(path='Vehicle.Speed'),
            types_pb2.DataEntry(path='Vehicle.ADAS.ABS.IsActive'),
        ])
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            entries = await client.get(entries=(
                EntryRequest('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
                EntryRequest('Vehicle.ADAS.ABS.IsActive', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
            ))

        assert entries == [DataEntry('Vehicle.Speed'), DataEntry('Vehicle.ADAS.ABS.IsActive')]

    @pytest.mark.usefixtures('val_server')
    async def test_get_nonexistent_entries(self, unused_tcp_port, val_servicer):
        error = types_pb2.Error(code=404, reason='not_found', message="Does.Not.Exist not found")
        errors = (types_pb2.DataEntryError(path='Does.Not.Exist', error=error),)
        val_servicer.Get.return_value = val_pb2.GetResponse(error=error, errors=errors)
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(VSSClientError):
                await client.get(entries=(
                    EntryRequest('Does.Not.Exist', View.CURRENT_VALUE, (Field.VALUE,)),
                ))

    @pytest.mark.usefixtures('val_server')
    async def test_set_some_updates(self, unused_tcp_port, val_servicer):
        val_servicer.Get.return_value = val_pb2.GetResponse(entries=(
            types_pb2.DataEntry(
                path='Vehicle.Speed', metadata=types_pb2.Metadata(data_type=types_pb2.DATA_TYPE_FLOAT),
            ),
            types_pb2.DataEntry(
                path='Vehicle.ADAS.ABS.IsActive',
                metadata=types_pb2.Metadata(data_type=types_pb2.DATA_TYPE_BOOLEAN),
            ),
            types_pb2.DataEntry(
                path='Vehicle.Cabin.Door.Row1.Left.Shade.Position',
                metadata=types_pb2.Metadata(data_type=types_pb2.DATA_TYPE_UINT8),
            ),
        ))
        val_servicer.Set.return_value = val_pb2.SetResponse()
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            await client.set(updates=[
                EntryUpdate(DataEntry('Vehicle.Speed', value=Datapoint(value=42.0)), (Field.VALUE,)),
                EntryUpdate(DataEntry(
                    'Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(value=False),
                ), (Field.ACTUATOR_TARGET,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    data_type=DataType.BOOLEAN,
                    entry_type=EntryType.SENSOR,
                    description="Indicates if cruise control system incurred and error condition.",
                    comment="No comment",
                    deprecation="Never to be deprecated",
                    unit=None,
                    value_restriction=None,
                )), (Field.METADATA,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    data_type=DataType.BOOLEAN,
                )), (Field.METADATA_DATA_TYPE,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    description="Indicates if cruise control system incurred and error condition.",
                )), (Field.METADATA_DESCRIPTION,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    entry_type=EntryType.SENSOR,
                )), (Field.METADATA_ENTRY_TYPE,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    comment="No comment",
                )), (Field.METADATA_COMMENT,)),
                EntryUpdate(DataEntry('Vehicle.ADAS.CruiseControl.Error', metadata=Metadata(
                    deprecation="Never to be deprecated",
                )), (Field.METADATA_DEPRECATION,)),
                EntryUpdate(DataEntry('Vehicle.Cabin.Door.Row1.Left.Shade.Position', metadata=Metadata(
                    unit='percent',
                )), (Field.METADATA_UNIT,)),
                EntryUpdate(DataEntry('Vehicle.Cabin.Door.Row1.Left.Shade.Position', metadata=Metadata(
                    value_restriction=ValueRestriction(min=0, max=100),
                )), (Field.METADATA_VALUE_RESTRICTION,)),
            ])
            assert val_servicer.Get.call_count == 1
            assert val_servicer.Get.call_args[0][0].entries == val_pb2.GetRequest(entries=(
                val_pb2.EntryRequest(path='Vehicle.Speed', view=View.METADATA, fields=(Field.METADATA_DATA_TYPE,)),
                val_pb2.EntryRequest(
                    path='Vehicle.ADAS.ABS.IsActive', view=View.METADATA, fields=(Field.METADATA_DATA_TYPE,),
                ),
                val_pb2.EntryRequest(
                    path='Vehicle.Cabin.Door.Row1.Left.Shade.Position',
                    view=View.METADATA,
                    fields=(Field.METADATA_DATA_TYPE,),
                ),
            )).entries
            assert val_servicer.Set.call_args[0][0].updates == val_pb2.SetRequest(updates=(
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.Speed', value=types_pb2.Datapoint(float=42.0),
                ), fields=(types_pb2.FIELD_VALUE,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.ABS.IsActive', actuator_target=types_pb2.Datapoint(bool=False),
                ), fields=(types_pb2.FIELD_ACTUATOR_TARGET,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(
                        data_type=types_pb2.DATA_TYPE_BOOLEAN,
                        entry_type=types_pb2.ENTRY_TYPE_SENSOR,
                        description="Indicates if cruise control system incurred and error condition.",
                        comment="No comment",
                        deprecation="Never to be deprecated",
                    ),
                ), fields=(types_pb2.FIELD_METADATA,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(data_type=types_pb2.DATA_TYPE_BOOLEAN),
                ), fields=(types_pb2.FIELD_METADATA_DATA_TYPE,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(
                        description="Indicates if cruise control system incurred and error condition."
                    ),
                ), fields=(types_pb2.FIELD_METADATA_DESCRIPTION,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(entry_type=types_pb2.ENTRY_TYPE_SENSOR),
                ), fields=(types_pb2.FIELD_METADATA_ENTRY_TYPE,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(comment="No comment"),
                ), fields=(types_pb2.FIELD_METADATA_COMMENT,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.ADAS.CruiseControl.Error',
                    metadata=types_pb2.Metadata(deprecation="Never to be deprecated"),
                ), fields=(types_pb2.FIELD_METADATA_DEPRECATION,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.Cabin.Door.Row1.Left.Shade.Position',
                    metadata=types_pb2.Metadata(unit="percent"),
                ), fields=(types_pb2.FIELD_METADATA_UNIT,)),
                val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                    path='Vehicle.Cabin.Door.Row1.Left.Shade.Position',
                    metadata=types_pb2.Metadata(value_restriction=types_pb2.ValueRestriction(
                        unsigned=types_pb2.ValueRestrictionUint(min=0, max=100),
                    )),
                ), fields=(types_pb2.FIELD_METADATA_VALUE_RESTRICTION,)),
            )).updates

    @pytest.mark.usefixtures('val_server')
    async def test_set_no_updates_provided(self, unused_tcp_port, val_servicer):
        val_servicer.Set.side_effect = generate_error(grpc.StatusCode.INVALID_ARGUMENT, 'No datapoints requested')
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(kuksa_client.grpc.VSSClientError) as exc_info:
                await client.set(updates=[])

            assert exc_info.value.args == kuksa_client.grpc.VSSClientError(error={
                'code': grpc.StatusCode.INVALID_ARGUMENT.value[0],
                'reason': grpc.StatusCode.INVALID_ARGUMENT.value[1],
                'message': 'No datapoints requested',
            }, errors=[]).args
            assert val_servicer.Get.call_count == 0
            assert val_servicer.Set.call_args[0][0].updates == val_pb2.SetRequest().updates

    @pytest.mark.usefixtures('val_server')
    async def test_set_nonexistent_entries(self, unused_tcp_port, val_servicer):
        error = types_pb2.Error(code=404, reason='not_found', message="Does.Not.Exist not found")
        errors = (types_pb2.DataEntryError(path='Does.Not.Exist', error=error),)
        val_servicer.Get.return_value = val_pb2.GetResponse(error=error, errors=errors)
        val_servicer.Set.return_value = val_pb2.SetResponse(error=error, errors=errors)
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(VSSClientError):
                await client.set(updates=(
                    EntryUpdate(DataEntry('Does.Not.Exist', value=Datapoint(value=42.0)), (Field.VALUE,)),),
                )

            assert val_servicer.Get.call_count == 1
            assert val_servicer.Set.call_count == 0
            with pytest.raises(VSSClientError):
                await client.set(updates=(
                    EntryUpdate(DataEntry(
                        'Does.Not.Exist',
                        value=Datapoint(value=42.0),
                        metadata=Metadata(data_type=DataType.FLOAT),
                    ), (Field.VALUE,)),),
                )

            assert val_servicer.Get.call_count == 1  # Get should'nt have been called again
            assert val_servicer.Set.call_count == 1

    @pytest.mark.usefixtures('val_server')
    async def test_authorize_not_implemented(self):
        with pytest.raises(NotImplementedError):
            await VSSClient('127.0.0.1', 424242).authorize(token='')

    @pytest.mark.usefixtures('val_server')
    async def test_subscribe_some_entries(self, mocker, unused_tcp_port, val_servicer):
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            responses = (
                # 1st response is subscription ack
                val_pb2.SubscribeResponse(updates=[
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.Speed',
                        value=types_pb2.Datapoint(
                            timestamp=timestamp_pb2.Timestamp(seconds=1667837915, nanos=247307674),
                            float=42.0,
                        ),
                    ), fields=(Field.VALUE,)),
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.ADAS.ABS.IsActive',
                        actuator_target=types_pb2.Datapoint(bool=True),
                    ), fields=(Field.ACTUATOR_TARGET,)),
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.Chassis.Height',
                        metadata=types_pb2.Metadata(
                            data_type=types_pb2.DATA_TYPE_UINT16,
                        ),
                    ), fields=(Field.METADATA_DATA_TYPE,)),
                ]),
                # Remaining responses are actual events that should invoke callback.
                val_pb2.SubscribeResponse(updates=[
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.Speed',
                        value=types_pb2.Datapoint(
                            timestamp=timestamp_pb2.Timestamp(seconds=1667837912, nanos=247307674),
                            float=43.0,
                        ),
                    ), fields=(Field.VALUE,)),
                ]),
                val_pb2.SubscribeResponse(updates=[
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.ADAS.ABS.IsActive',
                        actuator_target=types_pb2.Datapoint(bool=False),
                    ), fields=(Field.ACTUATOR_TARGET,)),
                ]),
                val_pb2.SubscribeResponse(updates=[
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.Chassis.Height',
                        metadata=types_pb2.Metadata(
                            data_type=types_pb2.DATA_TYPE_UINT8,
                        ),
                    ), fields=(Field.METADATA_DATA_TYPE,)),
                ]),
            )
            callback = mocker.Mock()
            val_servicer.Subscribe.return_value = (response for response in responses)

            sub_uid = await client.subscribe(entries=(entry for entry in (  # generator is intentional (Iterable)
                EntryRequest('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
                EntryRequest('Vehicle.ADAS.ABS.IsActive', View.TARGET_VALUE, (Field.ACTUATOR_TARGET,)),
                EntryRequest('Vehicle.Chassis.Height', View.METADATA, (Field.METADATA_DATA_TYPE,)),
            )), callback=callback)

            assert isinstance(sub_uid, uuid.UUID)
            while callback.call_count < 3:
                await asyncio.sleep(0.01)
            actual_updates = [list(call_args[0][0]) for call_args in callback.call_args_list]
            assert actual_updates == [
                [EntryUpdate(entry=DataEntry(path='Vehicle.Speed', value=Datapoint(
                    value=43.0,
                    timestamp=datetime.datetime(2022, 11, 7, 16, 18, 32, 247307, tzinfo=datetime.timezone.utc),
                )), fields=[Field.VALUE])],
                [EntryUpdate(entry=DataEntry(path='Vehicle.ADAS.ABS.IsActive', actuator_target=Datapoint(
                    value=False,
                )), fields=[Field.ACTUATOR_TARGET])],
                [EntryUpdate(entry=DataEntry(path='Vehicle.Chassis.Height', metadata=Metadata(
                    data_type=DataType.UINT8,
                )), fields=[Field.METADATA_DATA_TYPE])],
            ]

    @pytest.mark.usefixtures('val_server')
    async def test_subscribe_no_entries_requested(self, mocker, unused_tcp_port, val_servicer):
        val_servicer.Subscribe.side_effect = generate_error(
            grpc.StatusCode.INVALID_ARGUMENT, 'Subscription request must contain at least one entry.',
        )
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(VSSClientError):
                await client.subscribe(entries=(), callback=mocker.Mock())

    @pytest.mark.usefixtures('val_server')
    async def test_subscribe_nonexistent_entries(self, mocker, unused_tcp_port, val_servicer):
        val_servicer.Subscribe.side_effect = generate_error(grpc.StatusCode.INVALID_ARGUMENT, 'NotFound')
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(VSSClientError):
                await client.subscribe(entries=(entry for entry in (  # generator is intentional (Iterable)
                    EntryRequest('Does.Not.Exist', View.CURRENT_VALUE, (Field.VALUE,)),
                )), callback=mocker.Mock())

    @pytest.mark.usefixtures('val_server')
    async def test_unsubscribe(self, mocker, unused_tcp_port, val_servicer):
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            responses = (
                val_pb2.SubscribeResponse(updates=[
                    val_pb2.EntryUpdate(entry=types_pb2.DataEntry(
                        path='Vehicle.Speed',
                        value=types_pb2.Datapoint(
                            timestamp=timestamp_pb2.Timestamp(seconds=1667837915, nanos=247307674),
                            float=42.0,
                        ),
                    ), fields=(Field.VALUE,)),
                ]),
            )
            val_servicer.Subscribe.return_value = (response for response in responses)
            sub_uid = await client.subscribe(entries=(
                EntryRequest('Vehicle.Speed', View.CURRENT_VALUE, (Field.VALUE,)),
            ), callback=mocker.Mock())
            subscriber = client.subscribers.get(sub_uid)

            await client.unsubscribe(sub_uid)

            assert client.subscribers.get(sub_uid) is None
            assert subscriber.done()

            with pytest.raises(ValueError) as exc_info:
                await client.unsubscribe(sub_uid)
            assert exc_info.value.args[0] == f"Could not find subscription {str(sub_uid)}"

    @pytest.mark.usefixtures('val_server')
    async def test_get_server_info(self, unused_tcp_port, val_servicer):
        val_servicer.GetServerInfo.return_value = val_pb2.GetServerInfoResponse(name='test_server', version='1.2.3')
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            server_info = await client.get_server_info()
            assert server_info == ServerInfo(name='test_server', version='1.2.3')

    @pytest.mark.usefixtures('val_server')
    async def test_get_server_info_unavailable(self, unused_tcp_port, val_servicer):
        val_servicer.GetServerInfo.side_effect = generate_error(grpc.StatusCode.UNAVAILABLE, 'Unavailable')
        async with VSSClient('127.0.0.1', unused_tcp_port, ensure_startup_connection=False) as client:
            with pytest.raises(VSSClientError):
                await client.get_server_info()
