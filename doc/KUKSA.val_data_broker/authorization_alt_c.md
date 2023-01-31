# Authorization - Alternative C

Authorization is achived by supplying an `Authorization` header as part of the HTTP / GRPC requests.

`Authorization: Bearer TOKEN`

The bearer token is encoded as a JWT token, which means it contains everything the server needs to
verify the authenticity of the claims contained therein.

## Scopes
By specifying scopes when requesting a token (from a authorization server), the user of the token
can be limited in what they're allowed to do.

Example:

| Scope                  | Access                                        |
|------------------------|-----------------------------------------------|
|`read:Vehicle.Speed`    | Client can only read `Vehicle.Speed`          |
|`read:Vehicle.ADAS.*`   | Client can read everything under Vehicle.ADAS |
|`actuate:Vehicle.ADAS.*`| Client can actuate all actuators under Vehicle.ADAS |
|`provide:Vehicle.Width` | Client can provide `Vehicle.Width`            |

## Claims
The scopes specified when requesting the token will result in certain claims being available in
the resulting JWT token. These claims can then be used by the server to restrict what the client
can do.

The same basic design as [Alternative B](#alternative-b) but instead of having one `allow` and
one `deny` list, only one list exist where the order of the rules matter. First rule to match
is used.

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

