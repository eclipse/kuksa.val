#!/usr/bin/env python3

########################################################################
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


import argparse
import sys
from os import path

import json
import jwt


def error_exit(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


def createJWTToken(input_filename, priv_key, output_filename=None):
    print("Reading JWT payload from {}".format(input_filename))
    with open(input_filename, "r") as file:
        payload = json.load(file)

    encoded = jwt.encode(payload, priv_key, algorithm="RS256")

    if output_filename is None:
        output_filename = input_filename[:-5] if input_filename.endswith(".json") else input_filename
        output_filename += ".token"

    print("Writing signed access token to {}".format(output_filename))
    with open(output_filename, "w") as output:
        output.write(encoded)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("files", help="Read JWT payload from these files", nargs="+")
    script_dir = path.abspath(path.dirname(__file__))
    default_key_filename = path.join(script_dir, "jwt.key")

    parser.add_argument("--key", help="Private key location", dest="priv_key_filename", default=default_key_filename)
    parser.add_argument("--output", help="Name of the output file to store token to", dest="output")
    args = parser.parse_args()

    if args.output is not None and len(args.files) > 1:
        error_exit("""
        Both --output option and multiple files have been specified.
        Output filename can be specified for single input file only!
        """)

    print("Reading private key from {}".format("jwt.key"))
    with open(args.priv_key_filename, "r") as file:
        priv_key = file.read()

    for input_file in args.files:
        createJWTToken(input_file, priv_key, args.output)


if __name__ == "__main__":
    main()
