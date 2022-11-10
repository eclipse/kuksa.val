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

<<<<<<< HEAD
import threading
=======
import multiprocessing
>>>>>>> da7fe9e (fixup! Perform the stresstest)
import stress
import subscriber
import config
import argparse

def startStressing(cfg, file):
    _path = cfg.get("path")
    sub_num = int(cfg.get("subscriberCount"))
    _subscriber = []
    for i in range(0, sub_num):
        sub = multiprocessing.Process(target=subscriber.startSubscribe, args=(_path, file,))
        sub.start()
        _subscriber.append(sub)
    _message_num = int(cfg.get("messageNum"))
    _stresser = stress.stressServer(_message_num, _path, file,)
    for item in _subscriber:
        item.terminate()

def main():
    parser = argparse.ArgumentParser(description='Run stresstest with specific configuration')
    parser.add_argument('-f', '--file', action='store', default='config.ini',
                    help='The configuration file you want to use')
    args = parser.parse_args()
    cfg = config.getConfig(args.file)
    startStressing(cfg, args.file)
    print("Finished")
    

if __name__ == '__main__':
    main()
