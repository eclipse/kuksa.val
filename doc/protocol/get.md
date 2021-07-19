## get: Read data

KUKSA.val supports basic get requests according to VISS  validated against the following schema.

```json
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Get Request",
    "description": "Get the value of one or more vehicle signals and data attributes",
    "type": "object",
    "required": ["action", "path", "requestId" ],
    "properties": {
        "action": {
            "enum": [ "get" ],
            "description": "The identifier for the get request"
        },
        "path": {
             "$ref": "viss#/definitions/path"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
```


KUKSA.val supports VSS1 (dot-seperated) and VSS2 paths (slash-seperated) paths, e.g.

### VSS1 Example Request

```json 
{
    "action": "get", 
    "path": "Vehicle.Speed", 
    "requestId": "82176895204605218443916772566374508384", 
}
```

### Response


```json 
{
    "action": "get", 
    "path": "Vehicle.Speed", 
    "requestId": "82176895204605218443916772566374508384", 
    "ts": 1608121891, 
    "value": "100"
}
```

### VSS2 Example Request

The same works with VSS2 paths 
```json 
{
    "action": "get", 
    "path": "Vehicle/Speed", 
    "requestId": "52dd3aea-c311-42f3-9c90-d7421a7d1315", 
}
```

### Response


```json 
{
    "action": "get", 
    "data": {
        "dp": {
            "ts": "2021-07-13T20:27:17.1626204437Z", 
            "value": "200"
        }, 
        "path": "Vehicle/Speed"
    }, 
    "requestId": "52dd3aea-c311-42f3-9c90-d7421a7d1315", 
    "ts": "2021-07-13T20:33:12.1626204792Z"
}
```

## Wildcards
KUKSA.val supports the `*` wildcard in URIs. This wildcard matches a single subpath element, e.g. it will not recurse, unless it is at the end of a path. It will also __not__ match partial subpaths.

Wildcard matching will only expose nodes you have read access to.

Lets make a few examples to clarify

### Wildcard examples

__Query__
```json
{
    "action": "get", 
    "path": "Vehicle/*/Speed", 
    "requestId": "1ce80162-04b9-4ac4-b054-eecfcee72b4e"
}
```

__Response__
```json
{
    "action": "get", 
    "data": {
        "dp": {
            "ts": "1981-01-01T00:00:00.0000000000Z", 
            "value": "---"
        }, 
        "path": "Vehicle/OBD/Speed"
    }, 
    "requestId": "1ce80162-04b9-4ac4-b054-eecfcee72b4e", 
    "ts": "2021-07-13T20:36:16.1626204976Z"

}
```
Note, how this is _not_ matching `Vehicle/Speed`, as the match can not be empty. Likewise

__Query__
```json
{
    "action": "get", 
    "path": "Vehicle/Spee*", 
    "requestId": "f17c5fc6-2308-4f4d-8a51-2171b56b61e8"
}
```

```json
{
    "action": "get", 
    "error": {
        "message": "I can not find Vehicle/Spee* in my db", 
        "number": 404, 
        "reason": "Path not found"
    }, 
    "requestId": "f17c5fc6-2308-4f4d-8a51-2171b56b61e8", 
    "ts": "2021-07-13T20:37:57.1626205077Z"

}
```

does not match, as wildcards need to match complete path elements.

A more interesting example:

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door/*/*/IsLocked", 
    "requestId": "fcc7a27d-eda8-4bf9-ba7f-aa69120b1b88", 
```
 leads to 
<details>
  <summary>Response. Click to expand!</summary>

```json
{
    "action": "get", 
    "data": [
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row2/Left/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row2/Right/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row3/Left/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row3/Right/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row4/Left/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row4/Right/IsLocked"
        }
    ], 
    "requestId": "fcc7a27d-eda8-4bf9-ba7f-aa69120b1b88", 
    "ts": "2021-07-13T20:36:51.1626205011Z"
}
```
</details>

However


```json
{
    "action": "get", 
    "path": "Vehicle/*/IsLocked", 
    "requestId": "b8a5da94-0c2c-4f9c-8911-366cebff59f1"
}
```
gives 
```json
{
    "action": "get", 
    "error": {
        "message": "I can not find Vehicle/*/isLocked in my db", 
        "number": 404, 
        "reason": "Path not found"
    }, 
    "requestId": "b8a5da94-0c2c-4f9c-8911-366cebff59f1", 
    "ts": "2021-07-13T20:39:20.1626205160Z"
}
```

because `*` in  path matches only a single element.

A wildcard at the end of a path, does recurse, i.e.

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door/Row1/*", 
    "requestId": "06606a24-27e7-4717-b967-4211de1a5b00"
}
```
 
leads to 

<details>
  <summary>Response. Click to expand!</summary>

```json
{
    "action": "get", 
    "data": [
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/IsChildLockActive"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/IsOpen"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Shade/Position"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Shade/Switch"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Window/ChildLock"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Window/Position"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Window/Switch"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Left/Window/isOpen"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "0"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/LeftCount"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/IsChildLockActive"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/IsLocked"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/IsOpen"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Shade/Position"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Shade/Switch"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Window/ChildLock"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Window/Position"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Window/Switch"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "---"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/Right/Window/isOpen"
        }, 
        {
            "dp": {
                "ts": "1981-01-01T00:00:00.0000000000Z", 
                "value": "0"
            }, 
            "path": "Vehicle/Cabin/Door/Row1/RightCount"
        }
    ], 
    "requestId": "06606a24-27e7-4717-b967-4211de1a5b00", 
    "ts": "2021-07-13T20:42:22.1626205342Z"
}
```

</details>



The same effect can be achieved by querying a branch, i.e. the following query yields a similar result

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door/Row1", 
    "requestId": "06606a24-27e7-4717-b967-4211de1a5b00"
}
```

### Visibility

As mentioned, wildcard queries only return the visible subset depending on your authorization level. Consider using this token

```json

  "sub": "kuksa.val",
  "iss": "Eclipse KUKSA Dev",
  "admin": true,
  "iat": 1516239022,
  "exp": 1767225599,
  "kuksa-vss":  {
     "Vehicle.Powertrain.Transmission.DriveType": "r"
  }
  ```

```json
{
    "action": "get", 
    "path": "Vehicle/Speed",
    "requestId": "6d3ce665-4ad5-4ce7-9065-6d89f455e5e7", 
    "ts": "2021-07-13T20:44:38.1626205469Z"
}
```

```json
{
    "action": "get", 
    "error": {
        "message": "Insufficient read access to Vehicle/Speed", 
        "number": 403, 
        "reason": "Forbidden"
    }, 
    "requestId": "6d3ce665-4ad5-4ce7-9065-6d89f455e5e7", 
    "ts": "2021-07-13T20:44:39.1626205479Z"
}

```

While

```json
{
    "action": "get", 
    "requestId": "4927055b-0864-49f0-b2b8-20d18dc9eeb7", 
    "path": "Vehicle/Powertrain", 
}
````
 yields a result

```json
{
    "action": "get", 
    "data": {
        "dp": {
            "ts": "1981-01-01T00:00:00.0000000000Z", 
            "value": "unknown"
        }, 
        "path": "Vehicle/Powertrain/Transmission/DriveType"
    }, 
    "requestId": "4927055b-0864-49f0-b2b8-20d18dc9eeb7", 
    "ts": "2021-07-13T20:47:16.1626205636Z"
}
```

Wildcard expansion also respects visibility/access rights according to the token. Therefore, with the example token, the following paths would yield the same result
 * `Vehicle/Powertrain/Transmission/*`
 * `Vehicle/*/*/*` 
 * `Vehicle/*/*/DriveType`






