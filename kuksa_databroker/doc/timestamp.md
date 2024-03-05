# Databroker Timestamps
The implementation of KUKSA databroker shall represent the latest value of a ```Datapoint```. Therefore it adds a ```timestamp``` to a ```Datapoint```. If an actuator/provider/application has no system time the databroker sets the ```timestamp``` to the current system time. For some use cases it can be interesting to provide a timestamp set by the actuator/provider/application. For this we added a so called source timestamp (short ```source_ts```) to a ```Datapoint```. This source timestamp is optional and per default set to None.

# Potential attacks
If an attacker gets an authorized connection to the databroker he can set the source_timestamp and overwrite the value with a new one. But for this he/she needs read and write access through JWT tokens. If a provider decides to work with ```source_ts``` of a ```Datapoint``` than it should be clear that they can be false/outdated.
