# KUKSA.val TLS concept

This page describes the TLS support in KUKSA.val

## Security concept

KUKSA.val supports TLS for connection between KUKSA.val Databroker/Server and clients.

General design concept in short:

* KUKSA.val Server and KUKSA.val Databroker supports either connections secured with TLS or insecure connections.
* You can use configuration settings to control whether Server or databroker shall require secure connections.
* Default connection type may vary between tools, and may be changed in future releases.
* Mutual authentication is not supported, i.e. KUKSA.val Server and KUKSA.val Databroker does not authenticate clients
* A set of example certificates and keys exist in the [kuksa_certificates](kuksa_certificates) repository
* The example certificates are used as default by some applications
* The example certificates shall only be used during development and re not suitable for production use
* KUKSA.val does not put any additional requirements on what certificates that are accepted, default settings as defined by OpenSSL and gRPC are typically used

## Example certificates

For more information see the [README.md](kuksa_certificates/README.md).

**NOTE: The example keys and certificates shall not be used in your production environment!**

## Examples using example certificates

This section intends to give guidelines on how you can verify TLS functionality with KUKSA.val.
It is based on using the example certificates.


## KUKSA.val Databroker

KUKSA.val Databroker supports TLS, but not mutual authentication.
As of today, if TLS is not configured, KUKSA.val Databroker will accept insecure connections.

```
~/kuksa.val/kuksa_databroker$ cargo run --bin databroker -- --metadata ../data/vss-core/vss_release_4.0.json
```

The default behavior may change in the future. By that reason, it is recommended to use the `--insecure` argument
if you want to use insecure connections.

```
~/kuksa.val/kuksa_databroker$ cargo run --bin databroker -- --metadata ../data/vss-core/vss_release_4.0.json --insecure
```

To use a secure connection specify both `--tls-cert`and `--tls-private-key`

```
~/kuksa.val/kuksa_databroker$ cargo run --bin databroker -- --metadata ../data/vss-core/vss_release_4.0.json --tls-cert ../kuksa_certificates/Server.pem --tls-private-key ../kuksa_certificates/Server.key
```

Default certificates and keys are not included in the default KUKSA.val Databroker container,
so if running KUKSA.val Databroker from a default container you need to mount the directory containing the keys and certificates.

```
~/kuksa.val/kuksa_databroker$ docker run --rm -it  -p 55555:55555/tcp -v /home/user/kuksa.val/kuksa_certificates:/certs databroker --tls-cert /certs/Server.pem --tls-private-key /certs/Server.key
```

## KUKSA.val databroker-cli

Can be run in TLS mode like below.
Note that [databroker-cli](kuksa_databroker/databroker-cli/src/main.rs) currently expects the certificate
to have "Server" as subjectAltName.

```
~/kuksa.val/kuksa_databroker$ cargo run --bin databroker-cli -- --ca-cert ../kuksa_certificates/CA.pem
```

Default certificates and keys are not included in the default KUKSA.val Databroker-cli container,
so if running KUKSA.val Databroker-cli from a default container you need to mount the directory containing the keys and certificates.

```
docker run --rm -it --net=host -v /home/user/kuksa.val/kuksa_certificates:/certs databroker-cli --ca-cert /certs/CA.pem
```

## KUKSA.val Server

KUKSA.val Server uses TLS by default, but does not support mutual TLS.
By default it uses KUKSA.val example certificates/keys `Server.key`, `Server.pem` and `CA.pem`.

```
~/kuksa.val/kuksa-val-server/build/src$ ./kuksa-val-server  --vss ./vss_release_4.0.json
```

It is possible to specify a different certificate path, but the file names must be the same as listed above.

```
~/kuksa.val/kuksa-val-server/build/src$ ./kuksa-val-server  --vss ./vss_release_4.0.json -cert-path ../../../kuksa_certificates
```

In KUKSA.val Server the default certificates and keys are included in the container, so no need to
mount a directory if you want to use default certificates and keys.

```
docker run -it --rm -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL kuksa-val:latest
```

If using the default KUKSA.val Server Docker container there is no mechanism to use different certificates,
as the [Dockerfile](../kuksa-val-server/docker/Dockerfile) specifies that the default shall be used.

## KUKSA.val Client (command line)

See [KUKSA.val Client Documentation](../kuksa-client/README.md).

## KUKSA.val Client (library)

Clients like [KUKSA.val CAN Feeder](https://github.com/eclipse/kuksa.val.feeders/tree/main/dbc2val)
that use KUKSA.val Client library must typically set the path to the root CA certificate.
If the path is set the VSSClient will try to establish a secure connection.

```
# Shall TLS be used (default False for Databroker, True for KUKSA.val Server)
# tls = False
tls = True

# TLS-related settings
# Path to root CA, needed if using TLS
root_ca_path=../../kuksa.val/kuksa_certificates/CA.pem
# Server name, typically only needed if accessing server by IP address like 127.0.0.1
# and typically only if connection to KUKSA.val Databroker
# If using KUKSA.val example certificates the names "Server" or "localhost" can be used.
# tls_server_name=Server
```
