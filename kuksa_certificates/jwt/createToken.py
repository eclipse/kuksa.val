#!/usr/bin/env python3

# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

import json
import jwt
import argparse


def createJWTToken(input_filename, key):
    print("Reading JWT payload from {}".format(input_filename))
    with open(input_filename, "r") as file:
        payload = json.load(file)

    encoded = jwt.encode(payload, key, algorithm="RS256")

    if input_filename.endswith(".json"):
        output_filename = input_filename[:-5] + ".token"
    else:
        output_filename = output_filename + ".token"

    print("Writing signed key to {}".format(output_filename))
    with open(output_filename, "w") as output:
        output.write(encoded)


parser = argparse.ArgumentParser()
parser.add_argument("files", help="Read JWT payload from these files", nargs="+")
args = parser.parse_args()

print("Reading private key from {}".format("jwt.key"))
with open("jwt.key", "r") as file:
    key = file.read()

for input in args.files:
    createJWTToken(input, key)
