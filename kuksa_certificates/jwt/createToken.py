#!/usr/bin/python3

# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php

import json
import jwt
import argparse


def createJWTToken(input, key):
    print("Reading JWT payload from {}".format(input))
    with open(input,'r') as file:
        payload=json.load(file)
        
    encoded=jwt.encode(payload,key,algorithm='RS256')

    print("Writing signed key to {}".format(input+".token"))
    with open(input+".token",'w') as output:
        output.write(encoded.decode('utf-8'))




parser = argparse.ArgumentParser()
parser.add_argument("files", help="Read JWT payload from these files", nargs='+')
args = parser.parse_args()

print("Reading private key from {}".format('jwt.key'))
with open('jwt.key','r') as file:
    key=file.read()

for input in args.files:
    createJWTToken(input,key)






