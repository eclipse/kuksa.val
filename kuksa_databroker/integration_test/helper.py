# /********************************************************************************
# * Copyright (c) 2022 Contributors to the Eclipse Foundation
# *
# * See the NOTICE file(s) distributed with this work for additional
# * information regarding copyright ownership.
# *
# * This program and the accompanying materials are made available under the
# * terms of the Apache License 2.0 which is available at
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * SPDX-License-Identifier: Apache-2.0
# ********************************************************************************/

# Disable name checks due to proto generated classes
# pylint: disable=C0103

import asyncio
import logging
import os
import signal
import threading
from threading import Thread
from typing import Any, Callable, Dict, List, Mapping, Optional

import grpc
import pytest
from gen_proto.sdv.databroker.v1.broker_pb2 import (
    GetDatapointsRequest,
    GetMetadataRequest,
    SubscribeRequest,
)
from gen_proto.sdv.databroker.v1.broker_pb2_grpc import BrokerStub
from gen_proto.sdv.databroker.v1.collector_pb2 import (
    RegisterDatapointsRequest,
    RegistrationMetadata,
    UpdateDatapointsRequest,
)
from gen_proto.sdv.databroker.v1.collector_pb2_grpc import CollectorStub
from gen_proto.sdv.databroker.v1.types_pb2 import ChangeType, Datapoint, DataType

logger = logging.getLogger(__name__)


class Databroker:
    """
    Databroker wraps collector and broker APIs of the databroker.
    """

    def __init__(self, address: str) -> None:

        self._address = address

        logger.info("Databroker connecting to {}".format(self._address))
        # WARNING: always await grpc response!
        self._channel = grpc.aio.insecure_channel(self._address)  # type: ignore

        self._collector_stub = CollectorStub(self._channel)
        self._broker_stub = BrokerStub(self._channel)
        self._ids: Dict[str, int] = None  # type: ignore
        self._metadata = None

    async def close(self) -> None:
        """Closes runtime gRPC channel."""
        if self._channel:
            await self._channel.close()

    def __enter__(self) -> "Databroker":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        asyncio.run_coroutine_threadsafe(self.close(), asyncio.get_event_loop())

    async def __register_datapoints(self, datapoints: list):
        response = await self._collector_stub.RegisterDatapoints(
            RegisterDatapointsRequest(list=datapoints)
        )
        return response

    async def __update_datapoints(self, datapoints: Mapping[int, Datapoint]):
        response = await self._collector_stub.UpdateDatapoints(
            UpdateDatapointsRequest(datapoints=datapoints)
        )
        return response

    async def __get_datapoints(self, datapoints: list):
        response = await self._broker_stub.GetDatapoints(
            GetDatapointsRequest(datapoints=datapoints)
        )
        return response

    async def get_metadata(self, names=[]):
        """Requests metadata from databroker, allows for optional list of names

        Args:
            names (list, optional): List of names to get. Defaults to [].

        Returns:
            _type_: list, can be converted to json using parse_metadata()
        """
        response = await self._broker_stub.GetMetadata(
            GetMetadataRequest(names=names)
        )
        return response.list

    def metadata_to_json(self, metadata) -> list:
        """Parses metadata.list to json format

        Args:
            metadata (_type_): MetaDataReply.list

        Returns:
            list: Json objects
        """
        return [
            {
                "id": m.id,
                "name": m.name,
                "data_type": m.data_type,
                "description": m.description,
            }
            for m in metadata
        ]

    def datapoint_to_dict(self, name: str, dp: Datapoint) -> dict:
        """Convert Datapoint object to dictionary

        Args:
            name (str): Datapoint Name
            dp (Datapoint): Datapoint

        Returns:
            dict: with keys "name", "ts", "value", "type"
        """
        value_type = dp.WhichOneof("value")
        if value_type:
            # try to get directly dp.${which_one} attribute
            value = getattr(dp, value_type)
        ts = (
            dp.timestamp.seconds + int(dp.timestamp.nanos / 10**6) / 1000
        )  # round to msec
        result = {"name": name, "ts": ts, "value": value, "type": value_type}
        return result

    async def get_datapoints(self, datapoints=None):
        if datapoints is None:
            await self.__initialize_metadata()
            datapoints = self._ids.keys()

        response = await self.__get_datapoints(datapoints=datapoints)
        # map<string, Datapoint> datapoints = 1;
        return response

    async def __initialize_metadata(self, names=[]) -> None:
        if self._ids is None:
            self._ids = {}
            response = await self._broker_stub.GetMetadata([])
            self._metadata = response.list

            for item in response.list:
                self._ids[item.name] = item.id

    async def __get_or_create_datapoint_id_by_name(
        self, name: str, data_type: DataType
    ):
        await self.__initialize_metadata()

        key_list = self._ids.keys()
        if name not in key_list:
            response = await self.register_datapoint(name, data_type)
            datapoint_id = int(response)
            self._ids[name] = datapoint_id

        return self._ids[name]

    async def register_datapoint(self, name: str, data_type: DataType) -> int:
        await self.__initialize_metadata()

        registration_metadata = RegistrationMetadata()
        registration_metadata.name = name
        registration_metadata.data_type = data_type
        registration_metadata.description = ""
        registration_metadata.change_type = ChangeType.CONTINUOUS

        response = await self.__register_datapoints(datapoints=[registration_metadata])
        metadata_id = int(response.results[name])
        self._ids[name] = metadata_id
        return metadata_id

    async def set_int32_datapoint(self, name: str, value: int):
        datapoint = Datapoint()
        datapoint.int32_value = value
        datapoint_id = await self.__get_or_create_datapoint_id_by_name(
            name, DataType.INT32  # type: ignore
        )
        return await self.__update_datapoints({datapoint_id: datapoint})

    async def set_uint32_datapoint(self, name: str, value: int):
        datapoint = Datapoint()
        datapoint.uint32_value = value
        datapoint_id = await self.__get_or_create_datapoint_id_by_name(
            name, DataType.UINT32  # type: ignore
        )
        return await self.__update_datapoints({datapoint_id: datapoint})

    async def set_float_datapoint(self, name: str, value: float):
        datapoint = Datapoint()
        datapoint.float_value = value
        datapoint_id = await self.__get_or_create_datapoint_id_by_name(
            name, DataType.FLOAT  # type: ignore
        )
        return await self.__update_datapoints({datapoint_id: datapoint})

    def __get_grpc_error(self, err: grpc.RpcError) -> str:
        status_code = err.code()
        return "grpcError[Status:{} {}, details:'{}']".format(
            status_code.name, status_code.value, err.details()
        )

    async def subscribe_datapoints(
        self,
        query: str,
        sub_callback: Callable[[str, Datapoint], None],
        timeout: Optional[int] = None,
    ) -> None:
        try:
            request = SubscribeRequest(query=query)
            logger.info("broker.Subscribe('{}')".format(query))
            response = self._broker_stub.Subscribe(
                request, timeout=timeout
            )
            # NOTE:
            # 'async for' before iteration is crucial here with aio.channel!
            async for subscribe_reply in response:
                logger.debug("Streaming SubscribeReply %s", subscribe_reply)
                """ from broker.proto:
                message SubscribeReply {
                    // Contains the fields specified by the query.
                    // If a requested data point value is not available, the corresponding
                    // Datapoint will have it's respective failure value set.
                    map<string, databroker.v1.Datapoint> fields = 1;
                }"""
                if not hasattr(subscribe_reply, "fields"):
                    raise Exception("Missing 'fields' in {}".format(subscribe_reply))

                logger.debug("SubscribeReply.{}".format(subscribe_reply.fields))
                for name in subscribe_reply.fields:
                    dp = subscribe_reply.fields[name]
                    try:
                        logger.debug("Calling sub_callback({}, dp:{})".format(name, dp))
                        sub_callback(name, dp)
                    except Exception:
                        logging.exception("sub_callback() error", exc_info=True)
                        pass
            logger.debug("Streaming SubscribeReply done...")

        except grpc.RpcError as e:
            if (
                e.code() == grpc.StatusCode.DEADLINE_EXCEEDED
            ):  # expected code if we used timeout, just stop subscription
                logger.debug("Exitting after timeout: {}".format(timeout))
            else:
                logging.error(
                    "broker.Subscribe({}) failed!\n --> {}".format(
                        query, self.__get_grpc_error(e)
                    )
                )
                raise e
        except Exception:
            logging.exception("broker.Subscribe() error", exc_info=True)


def __on_subscribe_event(name: str, dp: Datapoint) -> None:
    value_type = dp.WhichOneof("value")
    if value_type:
        # try to get directly dp.${which_one} attribute
        value = getattr(dp, value_type)
    ts = (
        dp.timestamp.seconds + int(dp.timestamp.nanos / 10**6) / 1000
    )  # round to msec

    print(
        "#SUB# name:{}, value:{}, value_type:{}, ts:{}".format(
            name, value, value_type, ts
        ),
        flush=True,
    )


class SubscribeRunner:

    """
    Helper for gathering subscription events in a dedicated thread,
    running for specified timeout
    """

    def __init__(self, databroker_address: str, query: str, timeout: int) -> None:
        self.databroker_address = databroker_address
        self.query = query
        self.timeout = timeout
        self.thread: Optional[Thread] = None
        self.helper: Databroker
        self.events: Dict[str, Any] = {}

    def start(self) -> None:
        if self.thread is not None:
            raise RuntimeWarning("Thread %s already started!", self.thread.name)
        self.thread = Thread(
            target=self.__subscibe_thread_proc,
            name="SubscribeRunner({})".format(self.query),
        )
        self.thread.setDaemon(True)
        self.thread.start()

    def get_events(self):
        self.close()
        return self.events

    def get_dp_values(self, dp_name: str) -> List[Datapoint]:
        return [dp["value"] for dp in self.events[dp_name]]

    def find_dp_value(self, dp_name: str, dp_value: Any) -> Datapoint:
        if dp_name not in self.events:
            return None
        for dp in self.events[dp_name]:
            val = dp["value"]
            if val == dp_value:
                return dp
            elif isinstance(val, float) and dp_value == pytest.approx(val, 0.1):
                # allow 'fuzzy' float matching
                return dp
        return None

    def close(self) -> None:
        if self.thread is not None:
            logger.debug("Waiting for thread: %s", threading.current_thread().name)
            self.thread.join()
            self.thread = None

    async def __async_handler(self) -> Dict[str, Any]:
        def inner_callback(name: str, dp: Datapoint):
            """inner function for collecting subscription events"""
            dd = self.helper.datapoint_to_dict(name, dp)
            if name not in self.events:
                self.events[name] = []
            self.events[name].append(dd)

        # start client requests in different thread as subscribe_datapoints will block...
        logger.info("# subscribing('{}', timeout={})".format(self.query, self.timeout))
        try:
            await self.helper.subscribe_datapoints(
                self.query, timeout=self.timeout, sub_callback=inner_callback
            )
            return self.events
        except Exception as ex:
            logger.warning("Subscribe(%s) failed", self.query, exc_info=True)
            raise ex

    def __subscibe_thread_proc(self) -> None:
        # create dedicated event loop and instantiate Databroker in it
        logger.info("Thread %s started.", threading.current_thread().name)
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        logger.debug("Waiting %d sec for subscription [%s]", self.timeout, self.query)
        self.helper = Databroker(self.databroker_address)
        loop.run_until_complete(self.__async_handler())

        logger.debug(
            "Closing helper %d sec for subscription [%s]", self.timeout, self.query
        )
        loop.run_until_complete(self.helper.close())

        loop.close()
        logger.info("Thread %s finished.", threading.current_thread().name)


if __name__ == "__main__":
    LOOP = asyncio.get_event_loop()
    LOOP.add_signal_handler(signal.SIGTERM, LOOP.stop)
    LOOP.run_until_complete(main())
    LOOP.close()
