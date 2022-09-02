# Example Certificates and Tokens for KUKSA.val

This directory contains example keys, tokens and certificates that can be used for testing clients and servers in this repository.
Many of the clients and servers in this repository use keys, tokens and certificates from this directory by default.


## Keys and Certificates for TLS connections

The example root certificate is valid for 3650 days ( 10 years). Example client and server certificates are valid for 365 days (1 year).
If they have expired new certificates must be generated as described below.

### Generating Keys and Certificates for TLS Connections

Execute the script

```
> ./genCerts.sh
```

This creates `Client.pem` and `Server.pem` valid for 365 days since the day of generation.
If you want to also generate new keys, then delete the keys you want to regenerate before running the script.
This will trigger the script to generate new keys before generating the corresponding certificate.
If you want to regenerate `CA.pem` you must first delete it.

## Java Web Tokens (JWT)

KUKSA.val servers use Java Web Tokens (JWT) to validate that a client is authorized to read or write a specific datapoints.
Example keys and token exist in the [jwt](jwt) folder.
It contains a [jwt.key](jwt/jwt.key] file used to sign tokens and 3 example tokens.
Each example token exist both as a cleartext `*.json` file specifying content of the token as well as validity and a signed `*.json.token` file.

The following example tokens exist:

* [Single-read](jwt/single-read.json) that has read access to a single VSS signal
* [All-read-write](jwt/all-read-write.json) that can read and write all VSS signals
* [Super-admin](jwt/super-admin.json) that can read and write all VSS signals but also modify the signal tree

Note that the tokens have limited validity, if expired the `*.json` files need to be updated and the `*.json.token` files regenerated.

Two helper scripts exist for generating keys and tokens

* [recreateJWTkeyPair.sh](jwt/recreateJWTkeyPair.sh) to regenerate the JWT keys used for signing
* [createToken.py](jwt/createToken.py) to create signed tokens, requires `*.json` files as parameters

Example use:


```bash
$ pip3 install -r requirements.txt
$ ./createToken.py all-read-write.json
```
