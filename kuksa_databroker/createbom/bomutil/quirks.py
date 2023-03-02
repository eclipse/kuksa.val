#! /usr/bin/env python
########################################################################
# Copyright (c) 2022, 2023 Robert Bosch GmbH
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

'''Hook for applying some sanitation to make further processing easier'''

def apply_quirks(component):
    '''
    Takes one component entry from cargo license and might return
    a modified/extended entry.
    Use sparingly. Comment what you are doing
    Use narrow matching (name and complete license string) to catch
    changes
    '''
    if component["name"] in {"io-lifetimes", "linux-raw-sys", "rustix", "wasi"} \
        and component["license"] == "Apache-2.0 OR Apache-2.0 WITH LLVM-exception OR MIT":
        # All licenses are "OR", we already ship Apache-2.0 and MIT. The LLVM exception
        # does not apply to us, so lets keep it clean.
        component["license"] = "Apache-2.0 OR MIT"
        return component
    return component
