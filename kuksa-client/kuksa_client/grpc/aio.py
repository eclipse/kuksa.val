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
import logging
from typing import AsyncIterable
from typing import Callable
from typing import Collection
from typing import Dict
from typing import Iterable
from typing import List
import uuid

import grpc
from grpc.aio import AioRpcError

from kuksa.val.v1 import val_pb2
from kuksa.val.v1 import val_pb2_grpc

from . import BaseVSSClient
from . import Datapoint
from . import DataEntry
from . import DataType
from . import EntryRequest
from . import EntryUpdate
from . import Field
from . import Metadata
from . import MetadataField
from . import ServerInfo
from . import SubscribeEntry
from . import View
from . import VSSClientError

logger = logging.getLogger(__name__)


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

    async def get_current_values(self, paths: Iterable[str], **rpc_kwargs) -> Dict[str, Datapoint]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            current_values = await client.get_current_values([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ])
            speed_value = current_values['Vehicle.Speed'].value
        """
        entries = await self.get(
            entries=(EntryRequest(path, View.CURRENT_VALUE, (Field.VALUE,)) for path in paths),
            **rpc_kwargs,
        )
        return {entry.path: entry.value for entry in entries}

    async def get_target_values(self, paths: Iterable[str], **rpc_kwargs) -> Dict[str, Datapoint]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            target_values = await client.get_target_values([
                'Vehicle.ADAS.ABS.IsActive',
            ])
            is_abs_to_become_active = target_values['Vehicle.ADAS.ABS.IsActive'].value
        """
        entries = await self.get(entries=(
            EntryRequest(path, View.TARGET_VALUE, (Field.ACTUATOR_TARGET,),
            **rpc_kwargs,
        ) for path in paths))
        return {entry.path: entry.actuator_target for entry in entries}

    async def get_metadata(
        self, paths: Iterable[str], field: MetadataField = MetadataField.ALL, **rpc_kwargs,
    ) -> Dict[str, Metadata]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            metadata = await client.get_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ], MetadataField.UNIT)
            speed_unit = metadata['Vehicle.Speed'].unit
        """
        entries = await self.get(
            entries=(EntryRequest(path, View.METADATA, (Field(field.value),)) for path in paths),
            **rpc_kwargs,
        )
        return {entry.path: entry.metadata for entry in entries}

    async def set_current_values(self, updates: Dict[str, Datapoint], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            await client.set_current_values({
                'Vehicle.Speed': Datapoint(42),
                'Vehicle.ADAS.ABS.IsActive': Datapoint(False),
            })
        """
        await self.set(
            updates=[EntryUpdate(DataEntry(path, value=dp), (Field.VALUE,)) for path, dp in updates.items()],
            **rpc_kwargs,
        )

    async def set_target_values(self, updates: Dict[str, Datapoint], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            await client.set_target_values({'Vehicle.ADAS.ABS.IsActive': Datapoint(True)})
        """
        await self.set(updates=[EntryUpdate(
            DataEntry(path, actuator_target=dp), (Field.ACTUATOR_TARGET,),
        ) for path, dp in updates.items()], **rpc_kwargs)

    async def set_metadata(
        self, updates: Dict[str, Metadata], field: MetadataField = MetadataField.ALL, **rpc_kwargs,
    ) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            await client.set_metadata({
                'Vehicle.Cabin.Door.Row1.Left.Shade.Position': Metadata(data_type=DataType.FLOAT),
            })
        """
        await self.set(updates=[EntryUpdate(
            DataEntry(path, metadata=md), (Field(field.value),),
        ) for path, md in updates.items()], **rpc_kwargs)

    async def subscribe_current_values(
        self, paths: Iterable[str], callback: Callable[[Dict[str, Datapoint]], None], **rpc_kwargs,
    ) -> uuid.UUID:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
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
            **rpc_kwargs,
        )

    async def subscribe_target_values(
        self, paths: Iterable[str], callback: Callable[[Dict[str, Datapoint]], None], **rpc_kwargs,
    ) -> uuid.UUID:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
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
            **rpc_kwargs,
        )

    async def subscribe_metadata(
        self, paths: Iterable[str],
        callback: Callable[[Dict[str, Metadata]], None],
        field: MetadataField = MetadataField.ALL,
        **rpc_kwargs,
    ) -> uuid.UUID:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
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
            **rpc_kwargs,
        )

    async def get(self, *, entries: Iterable[EntryRequest], **rpc_kwargs) -> List[DataEntry]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        req = self._prepare_get_request(entries)
        try:
            resp = await self.client_stub.Get(req, **rpc_kwargs)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        return self._process_get_response(resp)

    async def set(self, *, updates: Collection[EntryUpdate], **rpc_kwargs) -> None:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        paths_with_required_type = self._get_paths_with_required_type(updates)
        paths_without_type = [
            path for path, data_type in paths_with_required_type.items() if data_type is DataType.UNSPECIFIED
        ]
        paths_with_required_type.update(await self.get_value_types(paths_without_type, **rpc_kwargs))
        req = self._prepare_set_request(updates, paths_with_required_type)
        try:
            resp = await self.client_stub.Set(req, **rpc_kwargs)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        self._process_set_response(resp)

    async def authorize(self, *, token: str) -> str:
        raise NotImplementedError('"authorize" is not yet implemented')

    async def subscribe(self, *,
        entries: Iterable[SubscribeEntry],
        callback: Callable[[Iterable[EntryUpdate]], None],
        **rpc_kwargs,
    ) -> uuid.UUID:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        req = val_pb2.SubscribeRequest()
        for entry in entries:
            entry_request = val_pb2.SubscribeEntry(path=entry.path, view=entry.view.value, fields=[])
            for field in entry.fields:
                entry_request.fields.append(field.value)
            req.entries.append(entry_request)
        logger.debug("%s: %s", type(req).__name__, req)
        resp_stream = self.client_stub.Subscribe(req, **rpc_kwargs)
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

    async def get_server_info(self, **rpc_kwargs) -> ServerInfo:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        req = val_pb2.GetServerInfoRequest()
        logger.debug("%s: %s", type(req).__name__, req)
        try:
            resp = await self.client_stub.GetServerInfo(req, **rpc_kwargs)
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

    async def get_value_types(self, paths: Collection[str], **rpc_kwargs) -> Dict[str, DataType]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        if paths:
            entry_requests = (EntryRequest(
                path=path, view=View.METADATA, fields=(Field.METADATA_DATA_TYPE,),
            ) for path in paths)
            entries = await self.get(entries=entry_requests, **rpc_kwargs)
            return {entry.path: DataType(entry.metadata.data_type) for entry in entries}
        return {}
