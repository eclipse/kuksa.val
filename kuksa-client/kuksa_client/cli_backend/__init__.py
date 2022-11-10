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

import kuksa_certificates

class Backend:
    def __init__(self, config):
        self.serverIP = config.get('ip', "127.0.0.1")
        self.serverPort = config.get('port', 8090)
        try:
            self.insecure = config.getboolean('insecure', False)
        except AttributeError:
            self.insecure = config.get('insecure', False)
        self.default_cert_path = pathlib.Path(kuksa_certificates.__path__[0])
        self.cacertificate = config.get('cacertificate', str(self.default_cert_path / 'CA.pem'))
        self.certificate = config.get('certificate', str(self.default_cert_path / 'Client.pem'))
        self.keyfile = config.get('key', str(self.default_cert_path / 'Client.key'))
        self.tokenfile = config.get('token', str(self.default_cert_path / 'jwt/all-read-write.json.token'))

    @staticmethod
    def from_config(config):
        protocol = config['protocol']

        # pylint: disable=cyclic-import,import-outside-toplevel
        if protocol == 'ws':
            from . import ws as backend_module
        elif protocol == 'grpc':
            from . import grpc as backend_module
        else:
            raise ValueError(f"Protocol {protocol!r} is not supported")
        # pylint: enable=cyclic-import,import-outside-toplevel

        return backend_module.Backend(config)
