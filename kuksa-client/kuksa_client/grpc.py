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
import json
from typing import Any
from typing import AsyncIterable
from typing import Callable
from typing import Iterable
from typing import Optional
from pathlib import Path
import uuid

from google.protobuf import json_format
import grpc
import jsonpath_ng

from kuksa.val import kuksa_pb2
from kuksa.val import kuksa_pb2_grpc


RequestType = enum.Enum('RequestType', kuksa_pb2.RequestType.items())


class VSSClientError(Exception):
    def __init__(self, statusCode: int, statusDescription: str):
        super().__init__(statusCode, statusDescription)
        self.statusCode = statusCode
        self.statusDescription = statusDescription

    @classmethod
    def from_status(cls, status: kuksa_pb2.Status):
        return cls(status.statusCode, status.statusDescription)


@dataclasses.dataclass
class VSSValue:
    val: Any
    path: Optional[str] = None
    timestamp: Optional[datetime.datetime] = None

    @classmethod
    def from_message(cls, message: kuksa_pb2.Value):
        message = json_format.MessageToDict(message, preserving_proto_field_name=True)
        path = message.pop('path', None)
        timestamp = message.pop('timestamp', None)
        value = next(iter(message.values()))
        if isinstance(value, str):
            try:
                value = json.loads(value)
            except json.JSONDecodeError:
                pass
        return cls(value, path, timestamp)


class VSSJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, VSSValue):
            return o.val
        return super().default(o)


class VSSClient:
    def __init__(
        self,
        host: str,
        port: int,
        root_certificates: Optional[Path] = None,
        private_key: Optional[Path] = None,
        certificate_chain: Optional[Path] = None,
    ):
        self.target_host = f'{host}:{port}'
        self.root_certificates = root_certificates
        self.private_key = private_key
        self.certificate_chain = certificate_chain
        self.metadata = ()
        self.channel = None
        self.client_stub = None
        self.exit_stack = contextlib.AsyncExitStack()
        self.subscribers = {}

    async def __aenter__(self):
        if all((self.root_certificates, self.private_key, self.certificate_chain)):
            root_certificates = self.root_certificates.read_bytes()
            private_key = self.private_key.read_bytes()
            certificate_chain = self.certificate_chain.read_bytes()
            creds = grpc.ssl_channel_credentials(root_certificates, private_key, certificate_chain)
            channel = grpc.aio.secure_channel(self.target_host, creds)
        else:
            channel = grpc.aio.insecure_channel(self.target_host)
        self.channel = await self.exit_stack.enter_async_context(channel)
        await self.channel.channel_ready()
        self.client_stub = kuksa_pb2_grpc.kuksa_grpc_ifStub(self.channel)
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        await self.exit_stack.aclose()
        self.client_stub = None
        self.channel = None

    async def get(self, *, type: RequestType, paths: Iterable[str]) -> Iterable[VSSValue]:
        req = kuksa_pb2.GetRequest(type=type.value, path=paths)
        resp = await self.client_stub.get(req, metadata=self.metadata)
        self.raise_if_invalid(resp)
        return [VSSValue.from_message(val) for val in resp.values]

    async def set(self, *, type: RequestType, values: Iterable[VSSValue]) -> None:
        req = kuksa_pb2.SetRequest(type=type.value)
        for value in values:
            dt = await self.get_data_type(value.path)
            val = kuksa_pb2.Value(path=value.path)

            value = value.val
            if dt in ('int8', 'int16', 'int32'):
                val.valueInt32 = int(value)
            elif dt in ('uint8', 'uint16', 'uint32'):
                val.valueUint32 = int(value)
            elif dt == 'uint64':
                val.valueUint64 = int(value)
            elif dt == 'int64':
                val.valueInt64 = int(value)
            elif dt == 'float':
                val.valueFloat= float(value)
            elif dt == 'double':
                val.valueDouble = float(value)
            elif dt == 'boolean':
                if value in ('False', 'false', 'F', 'f'):
                    value = 0
                val.valueBool = bool(value)
            else:
                val.valueString = str(value)
            req.values.append(val)
        resp = await self.client_stub.set(req, metadata=self.metadata)
        self.raise_if_invalid(resp)

    async def authorize(self, *, token: str) -> str:
        req = kuksa_pb2.AuthRequest(token=token)
        resp = await self.client_stub.authorize(req, metadata=self.metadata)
        self.raise_if_invalid(resp)
        self.metadata = (('connectionid', resp.connectionId),)
        return resp.connectionId

    async def subscribe(self, *,
        path: str,
        type: RequestType,
        callback: Callable[[VSSValue], None],
        start: bool,
    ) -> uuid.UUID:
        async def iterator(should_stop: asyncio.Event, req: kuksa_pb2.SubscribeRequest):
            yield req
            req.start = False
            await should_stop.wait()
            yield req

        should_stop = asyncio.Event()
        req = kuksa_pb2.SubscribeRequest(path=path, type=type.value, start=start)
        resp_stream = self.client_stub.subscribe(iterator(should_stop, req), metadata=self.metadata)
        # We expect the first SubscribeResponse to be immediately available and to only hold a status
        resp = await resp_stream.__aiter__().__anext__()
        self.raise_if_invalid(resp)

        sub_id = uuid.uuid4()
        new_sub_task = asyncio.create_task(self._subscriber_loop(message_stream=resp_stream, callback=callback))
        self.subscribers[sub_id] = (new_sub_task, should_stop)
        return sub_id

    async def unsubscribe(self, subscription_id: uuid.UUID):
        try:
            subscriber_task, should_stop = self.subscribers.pop(subscription_id)
        except KeyError as exc:
            raise ValueError(f"Could not find subscription {str(subscription_id)}") from exc
        subscriber_task.cancel()
        try:
            await subscriber_task
        except asyncio.CancelledError:
            pass
        should_stop.set()

    async def _subscriber_loop(
        self,
        *,
        message_stream: AsyncIterable[kuksa_pb2.SubscribeRequest],
        callback: Callable[[VSSValue], None],
    ):
        async for resp in message_stream:
            callback(VSSValue.from_message(resp.values))

    def raise_if_invalid(self, response):
        if response.status.statusCode != http.HTTPStatus.OK:
            raise VSSClientError.from_status(response.status)

    async def get_data_type(self, path: str):
        md = (await self.get(type=RequestType.METADATA, paths=(path,)))[0].val
        if "error" not in md:
            dt = [dt.value for dt in jsonpath_ng.parse("$..datatype").find(md)]
            if len(dt) == 1:
                datatype = dt[0]
                return datatype
        else:
            return None
