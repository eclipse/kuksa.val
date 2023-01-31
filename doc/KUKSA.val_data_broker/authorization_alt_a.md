# Authorization - Alternative A

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

Each claim represents an action and contains a list of paths to match.

If a paths starts with a `!`, the access rights normally granted by the claim are negated
for those matches (e.g. if contained in `read` would mean not allowed to read)

| Claim     | Description                                                      |
|-----------|------------------------------------------------------------------|
| `read`    | Paths to match when deciding access rights for reading signals   |
| `actuate` | Paths to match when deciding access rights for actuating signals |
| `provide` | Paths to match when deciding access rights for providing signals |


### Example 1

Allow reading and actuating all signals below Vehicle.ADAS.*.

Scope `"read:Vehicle.ADAS.* actuate:Vehicle.ADAS.*"` will be converted to the following claims in the JWT:
```
{
    ...
    "vss": {
        "read": [
            "Vehicle.ADAS.*"
        ],
        "actuate": [
            "Vehicle.ADAS.*"
        ]
    }
}
```

### Example 2

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

Scope `"read:Vehicle.ADAS.* read:!Vehicle.ADAS.Sensitive.*"` will be converted to the following
claims in the JWT:
```
{
    ...
    "vss": {
        "read": [
            "Vehicle.ADAS.*",
            "!Vehicle.ADAS.Sensitive.*"
        ]
    }
}
```

### Example 3

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping.*"`.

Scope `"read:Vehicle.Body.Windshield.*.Wiping.* provide:Vehicle.Body.Windshield.*.Wiping.*"` will
be converted to the following claims in the JWT:
```
{
    ...
    "vss": {
        "provide": [
            "Vehicle.Body.Windshield.*.Wiping.*"
        ]
    }
}
```
