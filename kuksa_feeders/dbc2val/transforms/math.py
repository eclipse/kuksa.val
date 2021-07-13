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

from py_expression_eval import Parser

class math:

    def __init__(self):
        self.parser=Parser()
        
    def transform(self, spec, value):
        return self.parser.parse(spec).evaluate({'x': value})
        