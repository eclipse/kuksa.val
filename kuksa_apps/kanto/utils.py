# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
#
# SPDX-License-Identifier: EPL-2.0

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
