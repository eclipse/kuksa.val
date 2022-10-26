#! /usr/bin/env python

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
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

__all__ = (
    "__title__",
    "__summary__",
    "__uri__",
    "__version__",
    "__author__",
    "__email__",
    "__license__",
    "__copyright__",
)

__title__ = "Unknown"
__summary__ = "Unknown"
__uri__ = "Unknown"
__version__ = "Unknown"
__author__ = "Unknown"
__email__ = "Unknown"
__license__ = "Unknown"
__copyright__ = "Copyright 2020 Robert Bosch GmbH"

import importlib_metadata

try:

    metadata = importlib_metadata.metadata("kuksa_client")


    __title__ = metadata["name"]
    __summary__ = metadata["summary"]
    __uri__ = metadata["home-page"]
    __version__ = metadata["version"]
    __author__ = metadata["author"]
    __email__ = metadata["author-email"]
    __license__ = metadata["license"]

except importlib_metadata.PackageNotFoundError as e:
    print(e)
    print("skip configuring metadata")
