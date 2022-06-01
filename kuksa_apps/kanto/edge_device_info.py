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
