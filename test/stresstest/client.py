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

import kuksa_client
import config

class Client:

    def start(self):
        clientType = self.cfg.get("protocol")
        if clientType == 'ws':
            print('Starting VISS client')
            self.connect()
            self.commThread.authorize()
        elif clientType == 'grpc':
            print('Starting gRPC client')
            self.connect()
        else: 
            raise ValueError(f"Unknown client type {clientType}")

    def connect(self):
        self.commThread = kuksa_client.KuksaClientThread(self.cfg)
        self.commThread.start()

    def __init__(self, file):
        self.cfg = config.getConfig(file)
        self.commThread = None
