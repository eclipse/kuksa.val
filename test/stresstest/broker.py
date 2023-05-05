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

import subprocess
import config
import signal
import argparse

class Broker:
    def start(self):
        port = self.cfg.get("port")
        address = self.cfg.get("ip")
        kuksa_val_databroker_cmd = ['cargo', 'run', '--release', '--bin', 'databroker', '--', '--metadata', '../../data/vss-core/vss_release_4.0.json', '--port', port, '--address', address]
        self.process = subprocess.check_call(kuksa_val_databroker_cmd)
    def __init__(self, config):
        self.cfg = config

def main():
    parser = argparse.ArgumentParser(description='Run stresstest with specific configuration')
    parser.add_argument('-f', '--file', action='store', default='config.ini',
                    help='The configuration file you want to use')
    args = parser.parse_args()
    cfg = config.getConfig(args.file)
    kuksa_val_databroker = Broker(cfg)
    kuksa_val_databroker.start()

if __name__ == '__main__':
    main()