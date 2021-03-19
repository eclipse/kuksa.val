## set: Write data

KUKSA.val supports basic set requests according to VISS validated against the [following schema](https://github.com/eclipse/kuksa.val/blob/master/include/VSSRequestJsonSchema.hpp#L48-L71):


```json
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Set Request",
    "description": "Enables the client to set one or more values once.",
    "type": "object",
    "required": ["action", "path", "value", "requestId"],
    "properties": {
        "action": {
            "enum": [ "set" ],
            "description": "The identifier for the set request"
        },
        "path": {
            "$ref": "viss#/definitions/path"
        },
        "value": {
            "$ref": "viss#/definitions/value"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
```

The following shows some valid and invalid sample requests. Unless otherwise noticed, we assume you are authorised to read the paths used in the examples.

### VSS1-Path Example Request

KUKSA.val supports VISS1 (dot-seperated) and VISS2 (slash-seperated) paths. VISS1 example:

```json 
{
  "action": "set",
  "path": "Vehicle.Speed",
  "value": "200",
  "requestId": "4204a97e-d25e-4ba9-891b-1703145865bd"
}
```

### Response

```json
{
    "action": "set", 
    "requestId": "4204a97e-d25e-4ba9-891b-1703145865bd", 
    "timestamp": 1615832573528
}
```

### VISS2-Path Example Request

```json
{
  "action": "set",
  "path": "Vehicle/Speed",
  "value": "200",
  "requestId": "8cedc7c8-089d-48d6-b913-9f1504c14342"
}
```

### Response

```json
{
    "action": "set", 
    "requestId": "8cedc7c8-089d-48d6-b913-9f1504c14342", 
    "timestamp": 1615832646622
}
```

## Wildcards/Multisets

Setting wildcards/multiple values or branches is currently not supported

### Try setting branch

```json
{
  "action": "set",
  "path": "Vehicle",
  "value": "200",
  "requestId": "3ef9e4c1-ff99-4b2c-be13-ce69bfb170b0"
}
```

### Response
 
```json
{
    "action": "set", 
    "error": {
        "message": "Can not set Vehicle. Only sensor or actor leaves can be set.", 
        "number": 403, 
        "reason": "Forbidden"
    }, 
    "requestId": "3ef9e4c1-ff99-4b2c-be13-ce69bfb170b0", 
    "timestamp": 1615832783387
}
```

### Try setting attribute
Attributes are considered a deployment attribute that does not change during the lifetime of a vehicle, therefore they can not be set using normal means. KUKSA.val assumes them to be initialized correctly by the loaded VSS JSON.

```json
{
  "action": "set",
  "path": "Vehicle/VehicleIdentification/VIN",
  "value": "deadbeef",
  "requestId": "4544fd5f-249e-4da6-8576-248b42504394"
}
```

### Response

```json
{
    "action": "set", 
    "error": {
        "message": "Can not set Vehicle/VehicleIdentification/VIN. Only sensor or actor leaves can be set.", 
        "number": 403, 
        "reason": "Forbidden"
    }, 
    "requestId": "4544fd5f-249e-4da6-8576-248b42504394", 
    "timestamp": 1615833005144
}

```

### Non-existing path
As expected, trying to set a non-existent path returns a 404

```json
{
  "action": "set",
  "path": "Vehicle/FluxCapacitor/Charge",
  "value": "1.21",
  "requestId": "882a6bdd-ff74-433a-a7c4-c2ce2c7f5c07"
}
```

### Response

```json
{
    "action": "set", 
    "error": {
        "message": "I can not find Vehicle/FluxCapacitor/Charge in my db", 
        "number": 404, 
        "reason": "Path not found"
    }, 
    "requestId": "882a6bdd-ff74-433a-a7c4-c2ce2c7f5c07", 
    "timestamp": 1615833326191
}
```

### Authorization Checks first
Please note, that authorization is checked *first*, i.e. in the previous example, if we do not have a permission covering the requested path, no matter it exists or not, we get a 403

```json
{
  "action": "set",
  "path": "Vehicle/FluxCapacitor/Charge",
  "value": "1.21",
  "requestId": "458a269f-ca42-4175-b88c-5ff8bea7855e"
}
```

### Response

```json
{
    "action": "set", 
    "error": {
        "message": "No write  access to Vehicle/FluxCapacitor/Charge", 
        "number": 403, 
        "reason": "Forbidden"
    }, 
    "requestId": "458a269f-ca42-4175-b88c-5ff8bea7855e", 
    "timestamp": 1615833458803
}
```

