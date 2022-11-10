<<<<<<< HEAD
=======
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

>>>>>>> a9a2d75 (fixup! Add a subscriber client to see the stress test and stress test subscriptions)
import client
import config

def debug_msg_callback(msg):
    print("processing")
    print(msg)

def msg_callback(msg):
    pass

def startSubscribe(path, file):
    cfg = config.getConfig(file)
    debug = bool(int(cfg.get("Debug")))
    Subscriber = client.Client(file)
    Subscriber.start()
    if debug:
        Subscriber.commThread.subscribe(path, debug_msg_callback)
    else:
        Subscriber.commThread.subscribe(path, msg_callback)