#!/usr/bin/env python3

# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

import argparse
import json
import jwt

from os import path


def createJWTToken(input_filename, priv_key):
    print("Reading JWT payload from {}".format(input_filename))
    with open(input_filename, "r") as file:
        payload = json.load(file)

    encoded = jwt.encode(payload, priv_key, algorithm="RS256")

    if input_filename.endswith(".json"):
        output_filename = input_filename[:-5] + ".token"
    else:
        output_filename = output_filename + ".token"

    print("Writing signed access token to {}".format(output_filename))
    with open(output_filename, "w") as output:
        output.write(encoded)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("files", help="Read JWT payload from these files", nargs="+")
    args = parser.parse_args()

    script_dir = path.abspath(path.dirname(__file__))
    priv_key_filename = path.join(script_dir, "jwt.key")

    print("Reading private key from {}".format("jwt.key"))
    with open(priv_key_filename, "r") as file:
        priv_key = file.read()

    for input in args.files:
        createJWTToken(input, priv_key)


if __name__ == "__main__":
    main()
