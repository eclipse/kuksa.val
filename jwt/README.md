# Databroker Example Tokens

This directory contains example tokens for demo and test purposes for KUKSA.val Databroker.
For more information on token format see [documentation](../doc/KUKSA.val_data_broker/authorization.md).

## Available tokens


* `actuate-provide-all.token` - gives access to set target value and actual value for all signals
* `provide-all.token` - gives access to set actual value for all signals, but not target value
* `read-all.token` - gives access to read actual and current value for all signals
* `provide-vehicle-speed.token` - gives access to write and read actual value for Vehicle.Speed. Does not give access to other signals
* `read-vehicle-speed.token` - gives access to read actual value for Vehicle.Speed. Does not give access to other signals


## Create new tokens

Tokens can be generated as described in [documentation](../kuksa_certificates/README.md).
Note that token generation must take place from the directory containing `createToken.py`

An example is shown below:

```
~/kuksa.val/kuksa_certificates/jwt$ python -m createToken ../../jwt/actuate-provide-all.json 
Reading private key from jwt.key
Reading JWT payload from ../../jwt/actuate-provide-all.json
Writing signed access token to ../../jwt/actuate-provide-all.token
```
