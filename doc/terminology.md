# Terminology

This pages gives an overview about the terms we use, when talking about KUKSA components or systems built with KUKSA.

* [Terminology](#terminology)
* [KUKSA.val components](#kuksaval-components)
   * [VSS Server](#vss-server)
   * [Client](#client)
   * [Clients: VSS Consumers](#clients-vss-consumers)
   * [Clients: VSS Providers](#clients-vss-providers)
      * [data-provider](#data-provider)
      * [actuation-provider](#actuation-provider)
* [Vehicle Signal Specification (VSS)](#vehicle-signal-specification-vss)
   * [Signal](#signal)
      * [Sensor](#sensor)
      * [Actuator](#actuator)
      * [Attribute](#attribute)
   * [Value](#value)
   * [Metadata](#metadata)
   * [Overlay](#overlay)
   * [Datapoint](#datapoint)

# KUKSA.val components

![In-vehicle KUKSA.val components](./pictures/terminology.svg)

KUKSA.val helps you provide and interact with VSS data in-vehicle. The figure above depicts the required components. We will describe each of their roles in the following sections. 

## VSS Server
This is a server that provides the KUKSA.val APIs to interact with VSS signals.
 
Usually, for KUKSA.val this is the KUKSA.val databroker. The server keeps the current state of all known VSS signals and enables clients to get information regarding those signals or changing their state. It is also responsible of for checking whether a client has sufficient authorization to access or modify certain signals.

Architecture-wise, most things in this document should apply to other VSS server implementations such as for example the older KUKSA.val val-server as well. (see [here](./server-vs-broker.md) for technical differences between these two VSS servers).

Databroker's API is GRPC-based, which means it can also be accessed over the network by other programs or systems.


## Client
Any application that connects to the server and uses the KUKSA.VAL APIs is a client.

From an architecture point of view, there are  different types of clients, depending on their role in the system.

The following picture shows a taxonomy of different client types, which will be discussed in the following. 

![KUKSA client taxonomy](./pictures/client-taxonomy.svg)

For specific examples of components fulfilling certain client roles also check [System Architecture](./system-architecture.md).

## Clients: VSS Consumers
VSS Consumers can also be seen as northbound clients of the VSS server. A VSS consumer intends to interact with the vehicle represented by the VSS model served by the server: A consumer typically wants to read sensors representing vehicle state available in VSS or affecting changes in the vehicle hardware by interacting with actuators available in VSS.

Examples for VSS consumers might be "apps" or functions running on vehicle computers, the infotainment system, or maybe even a user's personal device. 

## Clients: VSS Providers
VSS Providers can also be seen as southbound clients of the VSS server. A VSS provider intends to sync the state of the physical vehicle with the VSS model of the server.

Therefore, a VSS provider will always connect two interfaces: Northbound it uses the KUKSA.val API to interact with a VSS server, southbound it will interact with some other system, typically an in-vehicle bus or API. 

There are two classes of VSS providers: data-providers and actuation-providers. In practice, a single component might combine both roles. 

### data-provider
A data-provider intends to make sure that the actual state of a vehicle is currently represented in the VSS model of the server. A data-provider will update the current value of a VSS signal (sensor, actuator or attribute) in the server. A data-provider for the VSS sensor `Vehicle.Speed` will update that VSS signal in the server based on the actual speed of the vehicle. A data-provider for the VSS actuator `Vehicle.Body.Trunk.Rear.IsOpen` would update that VSS signal in the server based on the _currently_ observed state of the vehicle's trunk.
Historically you also may still find the term "feeder", when referring to a data-provider.

### actuation-provider
An actuation-provider is trying to ensure that the target value of a VSS actuator is reflected by the actual state of a vehicle. 

To this end, an actuation-provider can subscribe to the target value of a VSS actuator in the server. 
If a VSS consumer requests the _desired_ state of the VSS actuator `Vehicle.Body.Trunk.Rear.IsOpen` to be `true`, the actuation-provider for `Vehicle.Body.Trunk.Rear.IsOpen` would try to interact with a vehicle's system trying to unlock and open the trunk.

While from the server's point of view, an actuation provider is just a client, actuation-providers can not be passive towards other in-vehicle systems. Therefore, considering safety in an actuation-provider or underlying systems is very important. 

 # Vehicle Signal Specification (VSS)
 KUKSA handles datapoints that are defined using the COVESA Vehicle Signal Specification (VSS).

 VSS introduces a domain taxonomy for vehicle signals. It defines
 * A syntax for defining vehicle signals in a structured manner.
 * A catalog of standard signals (VSs standard catalogue) related to vehicles.

It can be used as standard in automotive applications to communicate information around the vehicle, which is semantically well defined. 

See the [the official VSS documentation](https://covesa.github.io/vehicle_signal_specification/) for all details. Here we will summarizes the terms and concepts most relevant for KUKSA.

## Signal
A signal in VSS is an entity identified by a dot-separated string in VSS, i.e. `Vehicle.Speed`.

A signal can be of different types. VSS defines the following signal types:

### Sensor 
Signals of this type represent sensors in the vehicle. The value of a sensor typically changes over time. Reading a sensor will return the current actual value of the related property, e.g. the current speed of a vehicle. Example: `Vehicle.Speed`. [[Source]](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/sensor_actuator/)

### Actuator
Actuators are signals that are  used to control the desired value of a property. _Every Actuator in VSS is also a Sensor_. Some properties in a vehicle cannot change instantly. A typical example is position of a seat. Reading a value of an actuator shall return the current actual value (i.e. the sensor trait of that actuator signal), e.g. the current position of the seat, rather than the wanted/desired position. Example: `set Vehicle.Cabin.Seat.Row1.Pos1.Position`. [[Source]](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/sensor_actuator/)

### Attribute  
Attributes are signals that have a default value, specified by its default member in VSS. Like sensors, attribute values can also change, similar to sensor values. The latter can be useful for attribute values that are likely to change during the lifetime of the vehicle. However, attribute values should typically not change more than once per ignition cycle. Example: `Vehicle.VehicleIdentification.VIN`. [[Source]](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/attributes/)

## Value
The value of a signal. The data type of the value must match the data type specified in the VSS entry for the signal. Currently KUKSA.val supports the _current_value_ for sensors, actuators and attributes as well as _target_value_ for actuators

## Metadata
Metadata of a VSS signal is data belonging to a signal, that is not the value. Standard VSS metadata are [unit](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/data_unit_types/) and [datatype](https://covesa.github.io/vehicle_signal_specification/rule_set/data_entry/data_types/) as well as some human readable description or comments. Custom metadata entries may be defined in [VSS overlays](https://covesa.github.io/vehicle_signal_specification/rule_set/overlay/). Currently KUKSA.val does not support custom metadata.

## Overlay
VSS has the concept of layering different signal trees on top of each other. This is called [overlay](https://covesa.github.io/vehicle_signal_specification/rule_set/overlay/). A layer may add signals to an existing VSS tree or extending and overriding existing signal entries.
Currently, KUKSA.val supports loading a stack of VSS models enabling applying overlays during startup.

## Datapoint
A datapoint is the value of a signal at a specific time (i.e. value + timestamp). This can generally be retrieved through the various APIs offered by databroker.