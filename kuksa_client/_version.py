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

from setuptools_git_versioning import version_from_git 

__version__ = version_from_git(
        template = "{tag}",
        dev_template = "{tag}-{ccount}",
        dirty_template =  "{tag}-{ccount}-dirty",
        starting_version =  "0.1.6"
        )

