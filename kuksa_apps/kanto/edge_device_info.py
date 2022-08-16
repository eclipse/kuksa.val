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


from ditto.model.namespaced_id import NamespacedID
import json

DEVICE_ID_KEY = "deviceId"
TENANT_ID_KEY = "tenantId"
POLICY_ID_KEY = "policyId"

ALLOWED_KEYS = [DEVICE_ID_KEY, TENANT_ID_KEY, POLICY_ID_KEY]


class EdgeDeviceInfo:
    def __init__(self, **kwargs):
        self.deviceId = None
        self.tenantId = None
        self.policyId = None

        for k, v in kwargs.items():
            print(k)
            if k in ALLOWED_KEYS:
                self.__setattr__(k, v)

    def unmarshal_json(self, data: json):
        try:
            envelope_dict = json.loads(data)
        except json.JSONDecodeError as jex:
            return jex

        for k, v in envelope_dict.copy().items():
            if k == DEVICE_ID_KEY:
                self.deviceId = NamespacedID().from_string(v)

            if k == POLICY_ID_KEY:
                self.policyId = NamespacedID().from_string(v)

            if k == TENANT_ID_KEY:
                self.tenantId = v

        print(self)
        return 0
