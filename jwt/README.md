# Databroker Example Tokens

*NOTE: The tokens here are copies of the tokens stored in [kuksa-common](https://github.com/eclipse-kuksa/kuksa-common/tree/main/jwt)!*


This directory contains example tokens for demo and test purposes for KUKSA.val Databroker.
For more information on token format see [documentation](../doc/KUKSA.val_data_broker/authorization.md).

## Available tokens


* `actuate-provide-all.token` - gives access to set target value and actual value for all signals
* `provide-all.token` - gives access to set actual value for all signals, but not target value
* `read-all.token` - gives access to read actual and current value for all signals
* `provide-vehicle-speed.token` - gives access to write and read actual value for Vehicle.Speed. Does not give access to other signals
* `read-vehicle-speed.token` - gives access to read actual value for Vehicle.Speed. Does not give access to other signals


## Create new tokens

Helper scripts and documentation on how to generate new keys and tokens exist in
[kuksa-common](https://github.com/eclipse-kuksa/kuksa-common/tree/main/jwt).
