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
from typing import AsyncIterator
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
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        await self.disconnect()

    async def connect(self, target_host=None):
        creds = self._load_creds()
        if target_host is None:
            target_host = self.target_host
        if creds is not None:
            channel = grpc.aio.secure_channel(target_host, creds)
        else:
            channel = grpc.aio.insecure_channel(target_host)
        self.channel = await self.exit_stack.enter_async_context(channel)
        self.client_stub = val_pb2_grpc.VALStub(self.channel)
        self.connected = True
        if self.ensure_startup_connection:
            logger.debug("Connected to server: %s", await self.get_server_info())

    async def disconnect(self):
        await self.exit_stack.aclose()
        self.client_stub = None
        self.channel = None
        self.connected = False

    def check_connected_async(func):
        async def wrapper(self, *args, **kwargs):
            if self.connected:
                return await func(self, *args, **kwargs)
            else:
                logger.info(
                    "Disconnected from server! Try cli command connect.")
        return wrapper

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
            entries=(EntryRequest(path, View.CURRENT_VALUE, (Field.VALUE,))
                     for path in paths),
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
            entries=(EntryRequest(path, View.METADATA, (Field(field.value),))
                     for path in paths),
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
            updates=[EntryUpdate(DataEntry(path, value=dp), (Field.VALUE,))
                     for path, dp in updates.items()],
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

    async def subscribe_current_values(self, paths: Iterable[str], **rpc_kwargs) -> AsyncIterator[Dict[str, Datapoint]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            async for updates in client.subscribe_current_values([
                'Vehicle.Speed', 'Vehicle.ADAS.ABS.IsActive',
            ]):
                for path, dp in updates.items():
                    print(f"Current value for {path} is now: {dp.value}")
        """
        async for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.CURRENT_VALUE, (Field.VALUE,))
                     for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.value for update in updates}

    async def subscribe_target_values(self, paths: Iterable[str], **rpc_kwargs) -> AsyncIterator[Dict[str, Datapoint]]:
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
        async for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.TARGET_VALUE,
                     (Field.ACTUATOR_TARGET,)) for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.actuator_target for update in updates}

    async def subscribe_metadata(
        self, paths: Iterable[str],
        field: MetadataField = MetadataField.ALL,
        **rpc_kwargs,
    ) -> AsyncIterator[Dict[str, Metadata]]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        Example:
            async for updates in client.subscribe_metadata([
                'Vehicle.Speed',
                'Vehicle.ADAS.ABS.IsActive',
            ]):
                for path, md in updates.items():
                    print(f"Metadata for {path} are now: {md.to_dict()}")
        """
        async for updates in self.subscribe(
            entries=(SubscribeEntry(path, View.METADATA, (Field(field.value),))
                     for path in paths),
            **rpc_kwargs,
        ):
            yield {update.entry.path: update.entry.metadata for update in updates}

    @check_connected_async
    async def get(self, *, entries: Iterable[EntryRequest], **rpc_kwargs) -> List[DataEntry]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            rpc_kwargs.get("metadata"))
        req = self._prepare_get_request(entries)
        try:
            resp = await self.client_stub.Get(req, **rpc_kwargs)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        return self._process_get_response(resp)

    @check_connected_async
    async def set(self, *, updates: Collection[EntryUpdate], **rpc_kwargs) -> None:
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
        paths_with_required_type.update(await self.get_value_types(paths_without_type, **rpc_kwargs))
        req = self._prepare_set_request(updates, paths_with_required_type)
        try:
            resp = await self.client_stub.Set(req, **rpc_kwargs)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        self._process_set_response(resp)

    async def subscribe(self, *,
                        entries: Iterable[SubscribeEntry],
                        **rpc_kwargs,
                        ) -> AsyncIterator[List[EntryUpdate]]:
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
                async for resp in resp_stream:
                    logger.debug("%s: %s", type(resp).__name__, resp)
                    yield [EntryUpdate.from_message(update) for update in resp.updates]
            except AioRpcError as exc:
                raise VSSClientError.from_grpc_error(exc) from exc
        else:
            logger.info("Disconnected from server! Try connect.")

    @check_connected_async
    async def authorize(self, token: str, **rpc_kwargs) -> str:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        rpc_kwargs["metadata"] = self.generate_metadata_header(
            metadata=rpc_kwargs.get("metadata"), header=self.get_authorization_header(token))
        req = val_pb2.GetServerInfoRequest()
        try:
            resp = await self.client_stub.GetServerInfo(req, **rpc_kwargs)
        except AioRpcError as exc:
            raise VSSClientError.from_grpc_error(exc) from exc
        logger.debug("%s: %s", type(resp).__name__, resp)
        self.authorization_header = self.get_authorization_header(token)
        return "Authenticated"

    @check_connected_async
    async def get_server_info(self, **rpc_kwargs) -> ServerInfo:
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
            resp = await self.client_stub.GetServerInfo(req, **rpc_kwargs)
            logger.debug("%s: %s", type(resp).__name__, resp)
            return ServerInfo.from_message(resp)
        except AioRpcError as exc:
            if exc.code() == grpc.StatusCode.UNAUTHENTICATED:
                logger.info("Unauthenticated channel started")
            else: 
                raise VSSClientError.from_grpc_error(exc) from exc
        return None

    async def get_value_types(self, paths: Collection[str], **rpc_kwargs) -> Dict[str, DataType]:
        """
        Parameters:
            rpc_kwargs
                grpc.*MultiCallable kwargs e.g. timeout, metadata, credentials.
        """
        if paths:
            entry_requests = (EntryRequest(
                path=path, view=View.METADATA, fields=(
                    Field.METADATA_DATA_TYPE,),
            ) for path in paths)
            entries = await self.get(entries=entry_requests, **rpc_kwargs)
            return {entry.path: DataType(entry.metadata.data_type) for entry in entries}
        return {}


class SubscriberManager:
    def __init__(self, client: VSSClient):
        self.client = client
        self.subscribers = {}

    async def add_subscriber(self,
                             subscribe_response_stream: AsyncIterator[List[EntryUpdate]],
                             callback: Callable[[Iterable[EntryUpdate]], None],
                             ) -> uuid.UUID:
        # We expect the first SubscribeResponse to be immediately available and to only hold a status
        await subscribe_response_stream.__aiter__().__anext__()  # pylint: disable=unnecessary-dunder-call

        sub_id = uuid.uuid4()
        new_sub_task = asyncio.create_task(
            self._subscriber_loop(subscribe_response_stream, callback))
        self.subscribers[sub_id] = new_sub_task
        return sub_id

    async def remove_subscriber(self, subscription_id: uuid.UUID):
        try:
            subscriber_task = self.subscribers.pop(subscription_id)
        except KeyError as exc:
            raise ValueError(
                f"Could not find subscription {str(subscription_id)}") from exc
        subscriber_task.cancel()
        try:
            await subscriber_task
        except asyncio.CancelledError:
            pass

    async def _subscriber_loop(
        self,
        subscribe_response_stream: AsyncIterator[List[EntryUpdate]],
        callback: Callable[[Iterable[EntryUpdate]], None],
    ):
        async for updates in subscribe_response_stream:
            callback(updates)
