## Alternative to using the "scope" claim
As part of working on a new design for the authorization in KUKSA.VAL, an updated
version of the "kuksa-vss" claim was at first envisioned.

One downside of a design like this is that it's not readily compatible with OAuth2,
which uses the "scope" claim to encode the same type of information.

A description of the envisioned claim is included here for reference.

In this proposed design, the claim consists of a list of rules, containing a set of actions
along with a set of paths they would apply to. In addition, the rule could be of
either a "allow" or "deny" type.


| Rule    | Description                                 |
|---------|---------------------------------------------|
| `allow` | A match will cause the action to be granted |
| `deny`  | A match will cause the action to be denied  |

The possible actions (initially) are:

| Action    | Description                                                          |
|-----------|----------------------------------|
| `read`    | Allow reading matching signals   |
| `actuate` | Allow actuating matching signals |
| `provide` | Allow providing matching signals |

To allow the creation of new signals, `create` could be an appropriate action name.

To allow editing the metadata of a signal `edit` could be an appropriate action name.

...

### Example 1

Allow reading and actuating all signals below Vehicle.ADAS.*.

Scope `"read:Vehicle.ADAS.* actuate:Vehicle.ADAS.*"` will be converted to the following claims
in the JWT:

```
{
    ...
    "vss": [
        {
            "rule": "allow",
            "actions": ["read", "actuate"],
            "paths": ["Vehicle.ADAS.*"]
        }
    ]
}
```

### Example 2

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

Scope `"!read:Vehicle.ADAS.Sensitive.* read:Vehicle.ADAS.*"` will be converted to the following
claims in the JWT:
```
{
    ...
    "vss": [
        {
            "rule": "deny",
            "actions": ["read"],
            "paths": ["Vehicle.ADAS.Sensitive.*"]
        },
        {
            "rule": "allow",
            "actions": ["read"],
            "paths": ["Vehicle.ADAS.*"]
        }
    ]
}
```

### Example 3

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping.*"`.

Scope `"read:Vehicle.Body.Windshield.*.Wiping.* provide:Vehicle.Body.Windshield.*.Wiping.*"` will
be converted to the following claims in the JWT:
```
{
    ...
    "vss": [
        {
            "rule": "allow",
            "actions": ["provide"],
            "paths": ["Vehicle.Body.Windshield.*.Wiping.*"]
        }
    ]
}
```

### Possible extension: using "access_mode" + "fields" for more granularity
The examples above use "actions" to specify what the rule allows/denies. These can be thought of as
a group of permissions needed to perform the intended action. But the access rights structure
enables replacing "actions" (and "paths") with something else, for example to express more granular
permissions.

Given this example using "actions":

```
"allow": [
    {
        "actions": ["read", "actuate"],
        "paths": ["Vehicle.*"]
    }
    ...
```

could be rewritten as a per-field permission rule instead:

```
"vss": [
    {
        "rule": "allow",
        "access_mode": "r",
        "fields": [
            "value"
        ],
        "paths": [
            "Vehicle.*"
        ]
    }
    {
        "rule": "allow",
        "access_mode": "rw",
        "fields": [
            "actuator_target"
        ],
        "paths": [
            "Vehicle.ADAS"
        ]
    }
    ...
```

The downside is that it quickly gets verbose. The point is that both ways of describing
access rights can coexist in the same list of rules. By taking advantage of combining
"allow" and "deny" rules, the verbosity could be minimised.

The scope requesting this could look like this:

```
"read:field:value:Vehicle.* readwrite:field:actuator_target:Vehicle.ADAS"
```


### Possible extension: Using "categories" instead of "paths"
Another possible extension is to allow something other than "paths" to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS .vspec files.
This information would be available as metadata for signals and could then be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the access rights structure as shown here.

This could be represented by specifying `tags` instead of `paths` in the rules, e.g.

```
"vss": [
    {
        "rule": "deny",
        "actions": ["read", "actuate"],
        "tags": ["restricted"]
    },
    {
        "rule": "allow",
        "actions": ["read", "actuate"],
        "tags": ["low_impact"]
    }
    ...
```

To request this using scopes, something like this could be used:
```
"!read:tag:restricted !actuate:tag:restricted read:tag:low_impact actuate:tag:low_impact"
```

