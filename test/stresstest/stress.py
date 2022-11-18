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

import client
import time
import config

def stressServer(num, path, file):
    print('stress testing kuksa.val server with setValue')
    Stressclient = client.Client(file)
    Stressclient.start()
    start = time.time_ns()
    for count in range(0, num):
        Stressclient.commThread.setValue('Vehicle.Speed', str(count))
    end = time.time_ns()
    elapsed_ns = end - start
    print('Elapsed time: %s ns' % elapsed_ns)
    one = (elapsed_ns / num)
    print('One message is sent all %s ns' % one)
    ratio = 1e9 / one
    print('Pushed %s total messages ' % num, '(%s / s)' % ratio)
    Stressclient.commThread.stop()
