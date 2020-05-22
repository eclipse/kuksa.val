#!/usr/bin/python3

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
########################################################################

import os

class elm2canbridge:
    def __init__(self,cfg):
        print("Try setting up elm2can bridge")
        print("Creating virtual CAN interface")
        os.system("./createelmcanvcan.sh")

