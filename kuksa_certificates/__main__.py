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
import os, sys, argparse 

from kuksa_viss_client._metadata import __version__

__certificate_dir__= os.path.dirname(os.path.realpath(__file__))
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--version", help="show program version", action="store_true")
    parser.add_argument("-j", "--jwt", help="show program version", action="store_true")
    args = parser.parse_args()

    if args.version:
        print ("Version " + str(__version__))
        return
    if args.jwt:
        print(os.path.join(__certificate_dir__, "jwt"))
    else:
        print(__certificate_dir__)

if __name__=="__main__":
    sys.exit(main())
