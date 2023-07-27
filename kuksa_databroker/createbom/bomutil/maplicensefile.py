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

"""Mapping of license identifiers of cargo license to the filenames of the actual license texts."""

MAP = {
    "Apache-2.0": "Apache-2.0.txt.gz",
    "BlueOak-1.0.0": "BlueOak-1.0.0.md.gz",
    "MIT": "MIT.txt.gz",
    "Unlicense": "Unlicense.txt.gz",
    "BSL-1.0": "BSL-1.0.txt.gz",
    "Unicode-DFS-2016": "Unicode-DFS-2016.txt.gz",
    "BSD-2-Clause": "BSD-2-Clause.txt.gz",
    "CC0-1.0": "CC0-1.0.txt.gz",
    "WTFPL": "WTFPL.txt.gz",
    "Zlib": "Zlib.txt.gz",
    "ISC": "ISC.txt.gz",
    "ring": "ring.LICENSE.txt.gz",
    "rustls-webpki": "webpki.LICENSE.txt.gz",
    # License text taken from https://spdx.org/licenses/0BSD.html
    "0BSD": "0BSD.txt.gz",
}
