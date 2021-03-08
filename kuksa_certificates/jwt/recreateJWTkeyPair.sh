#!/bin/bash

# Copyright Robert Bosch GmbH, 2020. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php



echo "Recreating kuksa.val key pair used for JWT verification"
echo "-------------------------------------------------------"


echo -e "\nCreating private key"
ssh-keygen -t rsa -b 4096 -m PEM -f jwt.key -q -N ""

echo -e "\nCreating public key"
openssl rsa -in jwt.key -pubout -outform PEM -out jwt.key.pub

echo -e '\nYou can use the PRIVATE key "jwt.key" to generate new tokens using https://jwt.io or the "createToken.py" script.'
echo 'You need to give the  PUBLIC key "jwt.key.pub" to the kuksa.val server, so it can verify correctly signed JWT tokens.'

