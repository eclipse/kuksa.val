#########################################################################
# Copyright (c) 2022 Bosch.IO GmbH
# Copyright (c) 2022 Contributors to the Eclipse Foundation
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


import json


# returns a key-value pairs where the key is the VSS path and the value is the
# value reported for this path
def process_tree(vss_tree):
    res = {}
    vss_data = json.loads(vss_tree)
    for item in vss_data['data']:
        print(item)
        res[item['path']] = item['dp']['value']
    print(res)
    return res


# returns a key-value pairs where the key is the VSS path and the value is the
# value reported for this path
def process_signal(vss_signal):
    res = {}
    vss_signal_json = json.loads(vss_signal)
    vss_data = vss_signal_json['data']
    return vss_data['path'], vss_data['dp']['value']
