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
    "requestId": "231543026388962994917867358932055791655", 
}
```

### Response


```json 
{
    "action": "get", 
    "path": "Vehicle/Speed", 
    "requestId": "231543026388962994917867358932055791655", 
    "ts": 1608121887, 
    "value": "100"
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
    "requestId": "103931565265046680818323224498857767104"
}
```

__Response__
```json
{
    "action": "get", 
    "path": "Vehicle/OBD/Speed", 
    "requestId": "103931565265046680818323224498857767104", 
    "ts": 1608377104, 
    "value": "100"
}
```
Note, how this is _not_ matching `Vehicle/Speed`, as the match can not be empty. Likewise

__Query__
```json
{
    "action": "get", 
    "path": "Vehicle/Spee*", 
    "requestId": "313075168675422297716541347645557507871" 
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
    "requestId": "313075168675422297716541347645557507871", 
    "ts": 1608377239
}
```

does not match, as wildcards need to match complete path elements.

A more interesting example:

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door/*/*/IsLocked", 
    "requestId": "311508011343524723835336189899358414689",
```
 leads to 
<details>
  <summary>Response. Click to expand!</summary>

```json
{
    "action": "get", 
    "requestId": "311508011343524723835336189899358414689", 
    "ts": 1608377507, 
    "value": [
        {
            "Vehicle/Cabin/Door/Row4/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/IsLocked": "---"
        }
    ]
}
```
</details>

However


```json
{
    "action": "get", 
    "path": "Vehicle/*/IsLocked", 
    "requestId": "45465648527119404680377594999611835141",
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
    "requestId": "45465648527119404680377594999611835141", 
    "ts": 1608377632
}
```

because `*` in  path matches only a single element.

A wildcard at the end of a path, does recurse, i.e.

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door/*", 
    "requestId": "221484597038784548242607265463583217704",
}
```
 
leads to 

<details>
  <summary>Response. Click to expand!</summary>

```json
{
    "action": "get", 
    "requestId": "221484597038784548242607265463583217704", 
    "ts": 1608377709, 
    "value": [
        {
            "Vehicle/Cabin/Door/Row4/Right/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Right/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row4/Left/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Right/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row3/Left/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Right/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row2/Left/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Right/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Window/isOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Window/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Window/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Window/ChildLock": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Shade/Switch": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/Shade/Position": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/IsOpen": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/IsLocked": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Row1/Left/IsChildLockActive": "---"
        }, 
        {
            "Vehicle/Cabin/Door/Count": 0
        }
    ]
}
```

</details>



The same effect can be achieved by querying a branch, i.e. the following query yields a similar result

```json
{
    "action": "get", 
    "path": "Vehicle/Cabin/Door", 
    "requestId": "221484597038784548242607265463583217704",
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
     "Vehicle.Drivetrain.Transmission.DriveType": "r"
  }
  ```

```json
{
    "action": "get", 
    "path": "Vehicle/Speed",
    "requestId": "68009472116763267974316500807917391601", 
    "ts": 1608378047
}
```

```json
  {
    "action": "get", 
    "error": {
        "message": "No read access to Vehicle.Speed", 
        "number": 403, 
        "reason": "Forbidden"
    }, 
    "requestId": "68009472116763267974316500807917391601", 
    "ts": 1608378047
}
```

While

```json
{
    "action": "get", 
    "requestId": "42417702951003321129172344096154765930", 
    "path": "Vehicle/Drivetrain", 
}
````
 yields a result

```json
{
    "action": "get", 
    "requestId": "42417702951003321129172344096154765930", 
    "ts": 1608378219, 
    "value": [
        {
            "Vehicle/Drivetrain/Transmission/DriveType": "unknown"
        }
    ]
}
```

Wildcard expansion also respects visibility/access rights according to the token. Therefore, with the example token, the following paths would yield the same result
 * `Vehicle/Drivetrain/Transmission/*`
 * `Vehicle/*/*/*` 
 * `Vehicle/*/*/DriveType`






