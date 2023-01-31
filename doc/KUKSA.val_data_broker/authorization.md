# Authorization

Authorization is achived by supplying an `Authorization` header as part of the HTTP / GRPC requests.

`Authorization: Bearer TOKEN`

The bearer token is encoded as a JWT token, which means it contains everything the server needs to verify the authenticity of the claims contained therein.

## Scopes
By specifying scopes when requesting a token (from a authorization server), the user of the token can be limited in what they're allowed to do.

Example:

| Scope                  | Access                                        |
|------------------------|-----------------------------------------------|
|`read:Vehicle.Speed`    | Client can only read `Vehicle.Speed`          |
|`read:Vehicle.ADAS.*`   | Client can read everything under Vehicle.ADAS |
|`actuate:Vehicle.ADAS.*`| Client can actuate all actuators under Vehicle.ADAS |
|`provide:Vehicle.Width` | Client can provide `Vehicle.Width`            |

## Claims
The scopes specified when requesting the token will result in certain claims being available in the resulting JWT token. These claims can then be used by the server to restrict what the client can do.


### Alternative A
| Claim             | Description                                    |
|-------------------|------------------------------------------------|
| `read_signals`    | Client is allowed to read these VSS signals    |
| `actuate_signals` | Client is allowed to actuate these VSS signals |
| `provide_signals` | Client is allowed to provide these VSS signals |


**Example 1**

Allow reading and actuating all signals below Vehicle.ADAS.*.

Scope `"read:Vehicle.ADAS.* actuate:Vehicle.ADAS.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "read_signals": [
        "Vehicle.ADAS.*"
    ],
    "actuate_signals": [
        "Vehicle.ADAS.*"
    ]
}
```

**Example 2**

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

Scope `"read:Vehicle.ADAS.* read:!Vehicle.ADAS.Sensitive.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "read_signals": [
        "Vehicle.ADAS.*",
        "!Vehicle.ADAS.Sensitive.*"
    ]
}
```

**Example 3**

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping.*"`.

Scope `"read:Vehicle.Body.Windshield.*.Wiping.* provide:Vehicle.Body.Windshield.*.Wiping.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "provide_signals": [
        "Vehicle.Body.Windshield.*.Wiping.*"
    ]
}
```

### Alternative B

An alternative design where a combination of "actions" and "paths" are combined into a "rule".
 
The basic claims are `allow` and `deny` which can be placed under the root claim `vss`.

| Claim   | Description                                                          |
|---------|----------------------------------------------------------------------|
| `allow` | Contains the rules governing what the client is allowed to do        |
| `deny`  | Contains the rules (overriding) what the client is allowed to do     |

All rules in `deny` will have precedence over what's in `allow`.

The possible actions (initially) are:

| Action    | Description                                                          |
|-----------|----------------------------------|
| `read`    | Allow reading matching signals   |
| `actuate` | Allow actuating matching signals |
| `provide` | Allow providing matching signals |

To allow the creation of new signals, `create` could be an appropriate action name.

To allow editing the metadata of a signal `edit` could be an appropriate action name.

To specify more granular permissions than this, see [possible extension](#possible-extension-using-mode--fields-for-more-granularity)


**Example 1**

Allow reading and actuating all signals below Vehicle.ADAS.*.

Scope `"read:Vehicle.ADAS.* actuate:Vehicle.ADAS.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "allow": [
        {
            "actions": ["read", "actuate"],
            "paths": ["Vehicle.ADAS.*"]
        }
    ]
}
```

**Example 2**

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

Scope `"read:Vehicle.ADAS.* read:!Vehicle.ADAS.Sensitive.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "allow": [
        {
            "actions": ["read"],
            "paths": ["Vehicle.ADAS.*"]
        }
    ],
    "deny": [
        {
            "actions": ["read"],
            "paths": ["Vehicle.ADAS.Sensitive.*"]
        }
    ]
}
```

**Example 3**

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping.*"`.

Scope `"read:Vehicle.Body.Windshield.*.Wiping.* provide:Vehicle.Body.Windshield.*.Wiping.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "allow": [
        {
            "actions": ["provide"],
            "paths": ["Vehicle.Body.Windshield.*.Wiping.*"]
        }
    ]
}
```

#### Possible extension: using "access_mode" + "fields" for more granularity

The examples above use "actions" to specify what the rule allows/denies. These can be thought of as a group of permissions needed to perform the intended action.
But the access rights structure enables replacing "actions" (and "paths") with something else, for example to express more granular permissions.

This example with "actions":

```
"allow": [
    {
        "actions": ["read", "actuate"],
        "paths": ["Vehicle.*"]
    }
    ...
```

could be rewritten to grant permissions on a per-field basis instead:

```
"allow": [
    {
        "access_mode": "r",
        "fields": [
            "value"
        ],
        "paths": [
            "Vehicle.*"
        ]
    },
    {
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
access rights can coexist in the same list of rules (theoretically at least).

The scope requesting this could look like this:
```
"read:field:value:Vehicle.* readwrite:field:actuator_target:Vehicle.ADAS"
```

#### Possible extension: Using "categories" instead of "paths"
Another possible extension is to allow something other than "paths" to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS .vspec files.
This information would be available as metadata for signals and could then be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the access rights structure as shown here.

This could be represented by specifying `categories` instead of `paths` in the rules, e.g.

```
"allow": [
    {
        "actions": ["read", "actuate"],
        "categories": ["low_impact"]
    }
],
"deny": [
    {
        "actions": ["read", "actuate"],
        "categories": ["restricted"]
    }
    ...
```

To request this using scopes, something like this could be used:
```
"read:category:low_impact actuate:category:low_impact !read:category:restricted !actuate:category:restricted"
```
