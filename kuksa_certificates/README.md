# Example Certificates and Tokens for KUKSA.val

This directory contains example keys, tokens and certificates that can be used for testing clients and servers in this repository.
Many of the clients and servers in this repository use keys, tokens and certificates from this directory by default.


## Keys and Certificates for TLS connections

This directory contain a set of example certificates, used by the KUKSA-project during development and testing.
They may or may not be useful for your test environment.
If needed you can customize `genCerts.sh` and generate keys and certificates that fits your environment.

See the [KUKSA.val TLS documentation](../tls.md) for general information on the KUKSA.val TLS concept.

This directory contains the following files with cryptographical information.

 Component      | Description |
| -------------- | ----------- |
| `CA.key` | Root key, tnot needed by KUKSA.val applications
| `CA.pem` | Root certificate, valid for 3650 days ( 10 years). |
| `Server.key` | Server key, needed by KUKSA.val Databroker/Server for TLS. |
| `Server.pem` | Server certificate chain, valid for 365 days, needed by KUKSA.val Databroker/Server for TLS. |
| `CLient.key` | Client key, currently not needed as mutual authentication is not supported. |
| `Server.pem` | Client certificate chain, valid for 365 days, currently not needed as mutual authentication is not supported. |

If the certificates have expired or you by any other reason need to regenerate keys or certificates you can use
the `genCerts.sh` cript as described below.

### Generating Keys and Certificates for TLS Connections

Execute the script

```
> ./genCerts.sh
```

This creates `Client.pem` and `Server.pem` valid for 365 days since the day of generation.
If you want to also generate new keys, then delete the keys you want to regenerate before running the script.
This will trigger the script to generate new keys before generating the corresponding certificate.
If you want to regenerate `CA.pem` you must first delete it.

**NOTE: The script genCerts.sh may not be suitable to use for generating keys and certificates for your production environment!  **
**NOTE: Please consult with your Project Security Manager (or similar) on how your keys and certificates shall be generated!  **

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
