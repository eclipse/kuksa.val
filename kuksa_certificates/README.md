# Example Certificates and Tokens for KUKSA.val

This directory contains example keys, tokens and certificates that can be used for testing clients and servers in this repository.
Many of the clients and servers in this repository use keys, tokens and certificates from this directory by default.

*Note that the primary storage location for keys, tokens and certificates usable for KUKSA Databroker*
*is [kuksa-common](https://github.com/eclipse-kuksa/kuksa-common/tree/main).*
*Files that also exist here should be considered as copies.*

## Keys and Certificates for TLS connections

This directory contain a set of example certificates, used by the KUKSA-project during development and testing.
They may or may not be useful for your test environment.
If needed you can customize the [genCerts.sh](https://github.com/eclipse-kuksa/kuksa-common/blob/main/tls/genCerts.sh) script and generate keys and certificates that fits your environment.

See the [KUKSA.val TLS documentation](../doc/tls.md) for general information on the KUKSA.val TLS concept.

This directory contains the following files with cryptographical information.

 Component      | Description |
| -------------- | ----------- |
| `CA.key` | Root key, tnot needed by KUKSA.val applications
| `CA.pem` | Root certificate, valid for 3650 days ( 10 years). |
| `Server.key` | Server key, needed by KUKSA.val Databroker/Server for TLS. |
| `Server.pem` | Server certificate chain, valid for 365 days, needed by KUKSA.val Databroker/Server for TLS. |
| `Client.key` | Client key, currently not needed as mutual authentication is not supported. |
| `Server.pem` | Client certificate chain, valid for 365 days, currently not needed as mutual authentication is not supported. |

If the certificates have expired or you by any other reason need to regenerate keys or certificates you can use
the [genCerts.sh](https://github.com/eclipse-kuksa/kuksa-common/blob/main/tls/genCerts.sh) as described in kuksa-common [documentation](https://github.com/eclipse-kuksa/kuksa-common/blob/main/tls/README.md).

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

Two helper scripts exist for generating keys and tokens, please see
[kuksa-common documentation](https://github.com/eclipse-kuksa/kuksa-common/tree/main/jwt)
