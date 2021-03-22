# KUKSA.VAL JWT authorization

The kuksa.val server uses JSON Web Tokens (JWT) to authorize clients.

Every kuksa.val client needs to provide a `token` that contains a description of the rights a specific client has. Different clients can have different access levels. When a cleint requests to read or write a ressource the kuksa.val server can check whether that client is permitted or not.

How does this work? First the access rights are describe in JSON, an example token payload to access all data looks like this:

```json
{
  "sub": "kuksa.val",
  "iss": "Eclipse KUKSA Dev",
  "admin": true,
  "iat": 1516239022,
  "exp": 1606239022,
  "modifyTree": true,
  "kuksa-vss":  {
     "*": "rw"
  }
}
``` 

This are the rules to create a valid kuksa.val token

1. The JWT Token should contain a "kuksa-vss" claim.
2. Under the "kuksa-vss" claim the permissions can be granted using key value pair. The key should be the path in the signal tree and the value should be strings with "r" for READ-ONLY, "w" for WRITE-ONLY and "rw" or "wr" for READ-AND-WRITE permission. See the image above.
3. The permissions can contain wild-cards. For eg "Vehicle.OBD.\*" : "rw" will grant READ-WRITE access to all the signals under Vehicle.OBD.
4. The permissions can be granted to a branch. For eg "Signal.Vehicle" : "rw" will grant READ-WRITE access to all the signals under Signal.Vehicle branch.
5. Optionally, you can also define the permissions for modifying tree using the `modifyTree` key. Set it to `true` to enable the modificationthe VSS tree and metadata in runtime.

 This is an example of another token that allows read access to OBD signals and write access to a single leaf

 ```json
{
  "sub": "kuksa.val",
  "iss": "Eclipse KUKSA Dev",
  "admin": true,
  "iat": 1516239022,
  "exp": 1696239022,
  "modifyTree": false,
  "kuksa-vss":  {
     "Vehicle.Drivetrain.Transmission.DriveType": "rw",
     "Vehicle.OBD.*": "r"
  }
}
 ``` 

The tokens are protected using public key cryptography using the RS256 mechanism. This is basically an RSA encrypted  SHA256 hash of the token contents. The private key is used to create the signature and the public key needs to be provided to the kuksa.val server so it can validate tokens. It will only accept tokens that are signed by the corresponding private key.

In [kuksa_certificates/jwt](../kuksa_certificates/jwt) is a helper script to create a valid token out of the json input like so 
```
$ ./createToken.py sometoken.json 
Reading private key from jwt.key
Reading JWT payload from sometoken.json
Writing signed key to sometoken.json.token
```

This uses `jwt.key` in the same folder as private key. The corresponding public key `jwt.key.pub` will be copied to the `build/src` folder during a cmake build.

If you wish to create a new key pair, [kuksa_certificates/jwt](../kuksa_certificates/jwt) contains a script for that
```
$ ./recreateJWTkeyPair.sh 
Recreating kuksa.val key pair used for JWT verification
-------------------------------------------------------

Creating private key
jwt.key already exists.
Overwrite (y/n)? y

Creating public key
writing RSA key

You can use the PRIVATE key "jwt.key" to generate new tokens using https://jwt.io or the "createToken.py" script.
You need to give the  PUBLIC key "jwt.key.pub" to the kuksa.val server, so it can verify correctly signed JWT tokens.
```


Another great ressource to play with and understand JWT tokens is https://jwt.io/. Use the RSA256 algorithm from the drop down. It can immediately decode all `*.token` files from kuksa.val.  Try it! When you copy the contents of `jwt.key.pub` to the public key textfield, it can check whether signature is valid, if you also copy the `jwt.key` to the private key textfield you can modify and create valid kuksa.val tokens directly on the webpage without the need for `createToken.py` script


![Alt text](.//pictures/jwt.png?raw=true "jwt")

To learn about JWT check these resources
* https://en.wikipedia.org/wiki/JSON_Web_Token
* https://tools.ietf.org/html/rfc7519
* https://jwt.io

## Advanced permission management
In a real deployment you might want to use Role based Access Controls. IN the kuksa.val architecture, we do not want the kuksa.val server on a vehicle to know about roles, to keep it simple. Instead we propose keeping roles in an IdM/IAM system such as [Keycloak](https://www.keycloak.org) in the backend. Such a system can manage roles and flatten them into tokens digestible by the kuksa.val server

We also envison use cases where maybe the environment an application runs in, already provides a basic level of access. For example an Android based IVI system might provide a token with basic set of read permissions to any application running within its sandbox.
