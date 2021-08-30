#!/usr/bin/python3

########################################################################
# Copyright (c) 2021 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################


class mapping:

    # setting discard_non_matching_items to true will return "None" if a value
    # is not found in the mapping table. This can be used to only add
    # a subset of possible values to VSS
    # setting discard_non_matching_items to false will return values for
    # which no match exists unmodified
    def __init__(self, discard_non_matching_items):
        self.discard_non_matching_items=discard_non_matching_items

    def transform(self, spec, value):
        for k,v in spec.items():
            if value==k:
                return v
        #no match
        if self.discard_non_matching_items:
            return None
        else:
            return value