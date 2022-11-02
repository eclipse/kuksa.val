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

import pathlib

import grpc.aio
import pytest
import pytest_asyncio

from kuksa.val.v1 import val_pb2_grpc

import tests


@pytest.fixture(name='resources_path', scope='function')
def resources_path_fixture():
    return pathlib.Path(tests.__path__[0]) / 'resources'


@pytest.fixture(name='val_servicer', scope='function')
def val_servicer_fixture(mocker):
    servicer = val_pb2_grpc.VALServicer()
    mocker.patch.object(servicer, 'Get', spec=True)
    mocker.patch.object(servicer, 'Set', spec=True)
    mocker.patch.object(servicer, 'Subscribe', spec=True)
    mocker.patch.object(servicer, 'GetServerInfo', spec=True)

    return servicer


@pytest_asyncio.fixture(name='val_server', scope='function')
async def val_server_fixture(unused_tcp_port, val_servicer):
    server = grpc.aio.server()
    val_pb2_grpc.add_VALServicer_to_server(val_servicer, server)
    server.add_insecure_port(f'127.0.0.1:{unused_tcp_port}')
    await server.start()
    try:
        yield server
    finally:
        await server.stop(grace=2.0)


@pytest_asyncio.fixture(name='secure_val_server', scope='function')
async def secure_val_server_fixture(unused_tcp_port, resources_path, val_servicer):
    server = grpc.aio.server()
    val_pb2_grpc.add_VALServicer_to_server(val_servicer, server)
    server.add_secure_port(f'localhost:{unused_tcp_port}', grpc.ssl_server_credentials(
        private_key_certificate_chain_pairs=[(
            (resources_path / 'test-server.key').read_bytes(),
            (resources_path / 'test-server.pem').read_bytes(),
        )],
        root_certificates=(resources_path / 'test-ca.pem').read_bytes(),
        require_client_auth=True,
    ))
    await server.start()
    try:
        yield server
    finally:
        await server.stop(grace=2.0)
