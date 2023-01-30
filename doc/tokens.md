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
  "kuksa-vss":  {
     "Vehicle.Speed": ["get_current"]
  }
}
``` 

The authorisation is handled by the `kuksa-vss` claim. In KUKSA.val we only handle explicit access rights, i.e. a token directly encodes what the owner of said token can do. 

As KUKSA.val is a rather low level component, we are not making assumptions about the security model around your application. Depending on your deployment, you maybe have Role-baed access models, or you work with bearer tokens. You need to make sure, that such access rights are rolled out into explicit descriptions by an IAM/IDM or some other form of policy database in your system.



## kuksa-vss claim format

Entries in the `kuksa_vss` claim, follow the following format

```json
"kuksa-vss":  {
     "<VSS_PATH>": ["<ACCESS_MODIFIER_1>", "<ACCESS_MODIFIER_2>", "..."]
  }
```

## VSS paths
Identifies to which path(s) the access modifiers apply to. A path may be a simple path to a signal such as (VSS  3.0 examples) `Vehicle.Speed`, or a path to a branch such as `Vehicle.Cabin`, implying the rule applies to all signals in that subtree

## Access Modifiers
Each path needs to contain a list of access modifiers, defining, which actions are allowed for that path. Currently defined access modifiers

### Reading
 * *get_current* : Allows reading (get/subscribe) of a signal's current value, e.g. the current `Vehicle.Speed`
 * *get_target*: Allows reading (get/subscribe) of a signal's target value, e.g. the desired state for `Vehicle.Cabin.Trunk.Front.IsOpen`
 * *get_meta*: Allows querying metadata.
 * *get_all*: Shortcut for *get_current*, *get_target* and *get_meta* 


### Writing
 * *set_current*: Allows writing (set) of a signal's current value, e.g. for providing the current `Vehicle.Speed`

 * *set_target*: Allows writing (set) of a signal's target value, e.g. for setting the desired state of `Vehicle.Cabin.Trunk.Front.IsOpen`.

 * *modify_model*: Allows changing metadata or even the tree structure (adding branches/signals) below a given path, e.g. `modify_model` rights for `Vehicle.FMS` would allow a client to add or change any number of elements under the `FMS` branch.


## Examples

## Superuser claim
No limits.

```json
"kuksa-vss":  {
     "Vehicle": ["get_all", "set_current", "set_target", "modify_model"],
  }
```

## OBD Provider
Suppose you have a provider for all signals available on OBD

```json
"kuksa-vss":  {
     "Vehicle.OBD": ["set_target"],
  }
```

## Battery SoC Service
An exampe, where a service queries data from the battery management system, to calculate a sate of charge.

```json
"kuksa-vss":  {
     "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current": ["set_current"],
     "Vehicle.Powertrain.TractionBattery.Temperature": ["get_current"],
     "Vehicle.Powertrain.TractionBattery.BMS": ["get_current"],
  }
```




## Order of evaluation
Entries are evaluated in order, the first entry that matches will be taken. If a application shall be able to read all `OBD` data and only write `OBD.Speed`, this configuration will not work

```json
"kuksa-vss":  {
     "Vehicle": ["get_all"]
  }
```

This token would not enable a client to set `Vehicle.OBD.Speed`, as the first entry already matches, and does not grant any set priviledges.

A correct token to achieve this looks like this

```json
"kuksa-vss":  {
     "Vehicle.OBD.Speed": ["get_all", "set_current" ],
     "Vehicle.OBD": ["get_all"],
}
```

As a mental model, structure you rules like you would with a firewall:  Specific rules first, generic "catch-all"s last.

If not matching rule if found, access is denied.

