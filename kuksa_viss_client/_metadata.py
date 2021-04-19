#! /usr/bin/env python

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
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

    metadata = importlib_metadata.metadata("kuksa_viss_client")


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

