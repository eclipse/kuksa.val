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
    "read_signals": [
        "Vehicle.Body.Windshield.*.Wiping.*"
    ],
    "provide_signals": [
        "Vehicle.Body.Windshield.*.Wiping.*"
    ]
}
```