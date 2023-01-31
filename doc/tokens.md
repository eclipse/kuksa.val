# Authorisaton in KUKSA.val

KUKSA.val offers you an efficient way to provide VSS signals in a vehicle computer. It aims to offer great flexibility how to use and configure it.

In most deployments, serveral entities may access KUKSA.val (different applications, services, probably even form other netwok nodes). In most cases you want different clients to have different levels of access to the VSS tree.

On a high level, these interactions with the VSS tree need to be considered

 * setting ("writing") signals: For existing datapoints, update the value, specifically this may be 
   * Updating the _current_value_ of a specific signal, e.g. a provider updating Vehicle speed 
   * Updating the _target_value_ of a an actuator signal, e.g. an application trying to open a vehicle's trunk by writing to the corresponding actuator
 * getting ("reading") signals: For existing datapoints, getting or subscribing the value, specifically this may be
   * Requesting the _current_value of a specific singal, e.g. an application subscribing to Vehicle Speed
   * Requesting the _target_value_ of an actuator signal, e.g. a provider subscribing to the _target_value_ indicating the intent to open or closing the trunk, in order to effect the change
 * Modifying the VSS tree itself. This may not be needed for all deployments, but for a typical exampe, consider adding a specfic trailer or implement to a vehicle, where some software components might want to register new datapoints under `Vehicle.Trailer` or `Vehicle.Harvester`.


 To describe the level of access for a given client KUKSA.val uses JSON Web Tokens (JWT) according to [RFC7519](https://www.rfc-editor.org/rfc/rfc7519).

 An example token may look like this

 ```json
{
  "sub": "kuksa.val",
  "iss": "Eclipse KUKSA Dev",
  "iat": 1516239022,
  "exp": 1606239022,
  "vss-read":  [
    "Vehicle.Speed"
  ]
}
``` 

The authorisation is handled by `vss-*` claims. In KUKSA.val we only handle explicit access rights, i.e. a token directly encodes what the owner of said token can do. 

As KUKSA.val is a rather low level component, we are not making assumptions about the security model around your application. Depending on your deployment, you maybe have Role-baed access models, or you work with bearer tokens. You need to make sure, that such access rights are rolled out into explicit descriptions by an IAM/IDM or some other form of policy database in your system.


## vss claims 

The following claims are supported.

### vss-read
This implies a client is able to read (get/subscribe) the current value for sensors, actuators and attributes, as well as the target value for actuators.

**Discussion: Might also be split into read-sensor, read-target? Should this also imply read-meta**

### vss-provide-sensor:
This implies a client is able to set the current value for sensors and actuators.

**Discussion: Might alternatively be called just "provide", which would fit providers that are "feeders", but  currently in my mind a "provider" could also be of the service variety, and be a component that reacts on updated target values. Might also be jsut "write-current" which may be more obvious if you ar enot into VSS/KUKSA lingo**

### vss-actuate
This implies a client is able to set the desired target value for actuators.

### vss-read-meta
This implies a client is able to read VSS metadata, including names of children, for the given paths. Reading meta data without any additonal rights is useful, if you want to give a means of discoverability to a client, wihtout neccesarily granting rights to acces the data itself-

### vss-modify-meta
This implies a client is able to change metadata of a signal

### vss-create-signals
This implies a client is able to create any number of new signals (sensors, actuators, attributes) including a branching structure under the given paths.
Create signals also implies modify-meta. **Discussion: or does it not?**



## VSS paths
Identifies to which path(s) a claim apply to. A path may be a simple path to a signal such as (VSS  3.0 examples) `Vehicle.Speed`, or a path to a branch such as `Vehicle.Cabin`, implying the rule applies to all signals in that subtree. An asterisk `*` can be used as wildcard.

### Examples
This examples are based on VSS 3.0
 * `Vehicle.Speed` identifies a single sensor
 * `Vehicle.OBD` identifes the branch `OBD`, and thus all rules aply to all succesors in the whole subtree
 * `Vehicle.OBD.*` applies to all children in the `OBD`, it is equivalent to the previous example **Discussion: We might also define this to not be recursive, i.e. match children signals, but not branches?**
 * `Vehicle.Body.Trunk.*.IsOpen` will match `Vehicle.Body.Trunk.Front.IsOpen` and `Vehicle.Body.Trunk.Rear.IsOpen`. The `*` only matches a single branch element, i.e. `Vehicle.*.IsOpen` would _not_ match  `Vehicle.Body.Trunk.Rear.AbsoluteLoad`, however `Vehicle.*.*.*.IsOpen` would
 * The `*` wildcard can only match full VSS path components, i.e. it can only applied after a dot, you can _not_ use `Vehicle.Spe*` to match `Vehicle.Speed`

## Examples

### Superuser claims
No limits.

```json
  "vss-read":  [
    "Vehicle"
   ],
   "vss-create-signals":  [
    "Vehicle"
   ],
   "vss-provide-sensor":  [
    "Vehicle"
   ],
   "vss-actuate":  [
    "Vehicle"
   ],
```

### OBD Provider
Suppose you have a provider for all signals available on OBD

```json
   "vss-provide-sensor":  [
    "Vehicle.OBD"
   ],
```

### Battery SoC Service
An exampe, where a service queries data from the battery management system, to calculate a sate of charge.

```json
   "vss-read":  [
    "Vehicle.Powertrain.TractionBattery.Temperature",
    "Vehicle.Powertrain.TractionBattery.BMS"
   ],
   "vss-provide-sensor":  [
    "Vehicle.Powertrain.TractionBattery.StateOfCharge"
   ],
```

### Trunk Provider for instances
An service that will provide current state of a vehicle's trunks as well as the ability to open the trunks.
It would use the read rights to subscribe to the target values, and report current status by means of the pthe privde-sensor claim for both front and rear trunks.

```json
   "vss-read":  [
    "Vehicle.Body.Trunk.*.IsOpen",
    "Vehicle.Body.Trunk.*.IsLocked",
   ],
   "vss-provide-sensor":  [
    "Vehicle.Body.Trunk.*.IsOpen",
    "Vehicle.Body.Trunk.*.IsLocked",
   ],
```

## Default policy is deny
If a path is not defined in a claim ocvering a specific action, the request is denied.