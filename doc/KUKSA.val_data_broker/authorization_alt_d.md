# Authorization - Alternative D

Authorization is achived by supplying an authorization header as part of any http2 / gRPC request.

`Authorization: Bearer TOKEN`

The bearer token is encoded as a JWT token, containing everything the server needs to verify
the authenticity of the claims contained therein.

## Scope
By specifying a scope when requesting an authorization token (from a authorization server), the user
of the token can be limited in what they're allowed to do.

The value of the scope parameter is expressed as a list of space-delimited, case-sensitive strings.

Examples:

| Scope                    | Access                                        |
|--------------------------|-----------------------------------------------|
|`read:Vehicle.Speed`      | Client can only read `Vehicle.Speed`          |
|`read:Vehicle.ADAS`       | Client can read everything under Vehicle.ADAS |
|`actuate:Vehicle.ADAS`    | Client can actuate all actuators under Vehicle.ADAS |
|`provide:Vehicle.Width`   | Client can provide `Vehicle.Width`            |
|`!read:Vehicle.Sensitive` | Client is _not_ allowed to read anything under `Vehicle.Sensitive` |

In order to specify the access rights for kuksa.val the following format is used.

| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`<ACTION>:<PATH>`      | Allow ACTION for PATH       |
|`!<ACTION>:<PATH>`     | Deny ACTION for PATH        |


with these available actions (initially):

| Action    | Description                                                          |
|-----------|----------------------------------|
| `read`    | Allow reading matching signals   |
| `actuate` | Allow actuating matching signals |
| `provide` | Allow providing matching signals |

To allow the creation of new signals, `create` could be an appropriate action name.

To allow editing the metadata of a signal `edit_metadata` could be an appropriate action name.

## Claims
The scope specified when requesting a token will result in certain claims being available in
the resulting JWT token. These claims can then be used by the server to restrict what the client
can do.

According to [RFC9068 - JWT Profile for OAuth 2.0 Access Tokens](https://datatracker.ietf.org/doc/html/rfc9068#section-2.2.3)

> [...] issued JWT access token SHOULD include a "scope" claim [...]

the "scope" claim should be present as a claim in the token.

By including it, the server has all the information it needs to perform access control.
Example of the claims available in the authorization token:

```
{
    "aud": ["kuksa.val.v1", "sdv.databroker.v1"],
    "iss": "https://issuer.example.com",
    "exp": 1443904177,
    "nbf": 1443904077,
    "sub": "dgaf4mvfs75Fci_FL3heQA",
    "client_id": "s6BhdRkqt3",
    "scope": "read:Vehicle actuate:Vehicle"
}
```

### Example 1

Allow reading and actuating all signals below `Vehicle.ADAS`.

```
{
    ...
    "scope": "read:Vehicle.ADAS actuate:Vehicle.ADAS"
}
```

### Example 2

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

```
{
    ...
    "scope": "!read:Vehicle.ADAS.Sensitive read:Vehicle.ADAS"
}
```

### Example 3

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping"`.

```
{
    ...
    "scope": "read:Vehicle.Body.Windshield.*.Wiping provide:Vehicle.Body.Windshield.*.Wiping"
}
```

### Possible extension: using "access_mode" + "fields" for more granularity
The examples above use "actions" to specify what a scope allows/denies. These can be thought of as
a group of permissions needed to perform the intended action. But it's possible to combine this
with an extended syntax that enables replacing "actions" (and "paths") with something else.

Possible future extensions:
| Scope                                | Description                                |
|--------------------------------------|--------------------------------------------|
|`<ACCESS_MODE>:field:<FIELD>:<PATH>`  | Allow ACCESS_MODE for field FIELD for PATH |
|`!<ACCESS_MODE>:field:<FIELD>:<PATH>` | Deny ACCESS_MODE for field FIELD for PATH  |

Specifying scopes in this way is more verbose, but allows finer grained control. The point is that
these scopes can coexist with the "normal" scopes. By taking advantage of combining "allow" and "deny"
rules, the verbosity can be minimised.

Example:
```
"read:field:value:Vehicle readwrite:field:actuator_target:Vehicle.ADAS"
```

### Possible extension: Using "tags" instead of "paths"
Another possible extension is to allow something other than "paths" to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS .vspec files.
This information would be available as metadata for signals and could then be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the scope syntax as shown here.

Possible future extensions:
| Scope                                    | Description                                                   |
|------------------------------------------|---------------------------------------------------------------|
|`<ACCESS_MODE>:field:<FIELD>:tag:<PATH>`  | Allow ACCESS_MODE for field FIELD for signals tagged with TAG |
|`!<ACCESS_MODE>:field:<FIELD>:tag:<PATH>` | Deny ACCESS_MODE for field FIELD for signals tagged with TAG  |
|`<ACTION>:tag:<TAG>`                      | Allow ACTION for signals tagged with TAG                      |
|`!<ACTION>:tag:<TAG>`                     | Deny ACTION for signals tagged with TAG                       |

Example:

```
"!read:tag:restricted !actuate:tag:restricted read:tag:low_impact actuate:tag:low_impact"
```

