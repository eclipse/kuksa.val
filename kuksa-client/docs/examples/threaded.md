# Thread-based client

`kuksa_client.KuksaClientThread` provides a thread-based client that supports both `ws` and `grpc` to interact with either `kuksa-val-server` or `kuksa_databroker`.
This API allows a blocking application such as kuksa-client's CLI to run on its own thread and have the client/server communication happening on its own thread.
Be aware though that this implementation is not well suited for performance critical applications because of the single-threaded nature of the Python interpreter.
Be also aware that this API returns JSON responses whose schema may vary from one server to the other.

## Usage

`kuksa_client.KuksaClientThread` accepts a configuration dictionary. The following properties for the connection can be configured:
- `ip` server/databroker hostname or IP address, default: "127.0.0.1"
- `port` server/databroker port, default: 8090
- `protocol` protocol used to interact with server/databroker ("ws" or "grpc"), default: "ws"
- `insecure` whether the communication should be unencrypted or not, default: `False`
- `cacertificate` root certificate path, default: "../kuksa_certificates/CA.pem"
- `certificate` client certificate path, default: "../kuksa_certificates/Client.pem"
- `key` client private key path, default: "../kuksa_certificates/Client.key"

```python
# An empty configuration dictionary will use the aforementioned default values:
config = {}
# Here is what a databroker configuration would look like:
config = {
    'port': 55555,
    'protocol': 'grpc',
    'insecure': True,  # databroker does not yet support encryption
}
client = kuksa_client.KuksaClientThread(config)
```

Here's the simplest example how one can retrieve vehicle's current speed from `kuksa-val-server`:

```python
import json

import kuksa_client

client = kuksa_client.KuksaClientThread(config={})
client.start()
client.authorize()

response = json.loads(client.getValue('Vehicle.Speed'))
print(response['data']['dp']['value'])

client.stop()
```

Here's the simplest example how one can retrieve vehicle's current speed from `kuksa_databroker`:

```python
import json

import kuksa_client

client = kuksa_client.KuksaClientThread(config={'protocol': 'grpc', 'port': 55555, 'insecure': True})
client.start()
# next two lines only needed if you want to use authorization
token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZSJ9.OJWzTvDjcmeWyg3vmBR5TEtqYaHq8HrpFLlTKZAfDBAQBUHpyUEboJ97jfWuWgBnTpnfboyfAbwvLqo6bEVZ6tXzF8n9LtW6HmPbIWoDqXuobM2grUCVaGKuOcnCpMCQYChziqHbYwRJYP9nkYgbQU1kE4dN7880Io4xzq0GEbWksB2CVpOoExQUmCZpCohPs-XEkdmXhcUKnWnOeiSsRGKusx987vpY_WOXh6WE7DfJgzAgpPDo33qI7zQuTzUILORQsiHmsrQO0-zcvokNjaQUzlt5ETZ7MQLCtiUQaN0NMbDMCWkmSfNvZ5hKCNbfr2FaiMzrGBOQdvQiFo-DqZKGNweaGpufYXuaKfn3SXKoDr8u1xDE5oKgWMjxDR9pQYGzIF5bDXITSywCm4kN5DIn7e2_Ga28h3rBl0t0ZT0cwlszftQRueDTFcMns1u9PEDOqf7fRrhjq3zqpxuMAoRANVd2z237eBsS0AvdSIxL52N4xO8P_h93NN8Vaum28fTPxzm8p9WlQh4mgUelggtT415hLcxizx15ARIRG0RiW91Pglzt4WRtXHnsg93Ixd3yXXzZ2i4Y0hqhj_L12SsXunK2VxKup2sFCQz6wM-t_7ADmNYcs80idzsadY8rYKDV8N1WqOOd4ANG_nzWa86Tyu6wAwhDVag5nbFmLZQ"
client.authorize(token_or_tokenfile=token)

response = json.loads(client.getValue('Vehicle.Speed'))
print(response['value']['value'])

client.stop()
```

## Return Values

What will be returned and in what form is not well defined, and differs between KUKSA.val Server and KUKSA.val Databroker.
This gives problem if writing an application that shall support both KUKSA.val Server and KUKSA.val Databroker.
The example flows below highlight some of the differences.

If you intend to support both KUKSA.val Server and KUKSA.val Databroker any only is interested in whether a call
succeeded or not you can use the following approach.

Check if the response is `OK` - This is only returned by KUKSA.val Databroker but if present call has succeded.
If not it can be assumed that response is JSON.
Then do a `resp = json.loads(response)` and then check `if "error" in resp`. If no error is found it can be assumed
that the call succeded.

 resp = json.loads(self._kuksa.getMetaData(vss_name))
        if "error" in resp:


### KUKSA.val Databroker

```
Test Client> setValue Vehicle.Speed 54
OK

Test Client> getValue Vehicle.Speed
{
    "path": "Vehicle.Speed",
    "value": {
        "value": 54.0,
        "timestamp": "2023-06-13T09:17:09.103507+00:00"
    }
}

Test Client> getValue Vehicle.Zpeed
{
    "error": {
        "code": 404,
        "reason": "not_found",
        "message": "Path not found"
    },
    "errors": [
        {
            "path": "Vehicle.Zpeed",
            "error": {
                "code": 404,
                "reason": "not_found",
                "message": "Path not found"
            }
        }
    ]
}

# Next is an example on what is returned if Databroker instance has been stopped

Test Client> getValue Vehicle.Speed
{
    "error": {
        "code": 14,
        "reason": "unavailable",
        "message": "failed to connect to all addresses; last error: UNKNOWN: ipv4:127.0.0.1:55555: Failed to connect to remote host: Connection refused"
    },
    "errors": []
}
```

### KUKSA.val Server

```
Test Client> setValue Vehicle.Speed 54
{
  "action": "set",
  "requestId": "b0b48698-f747-41f8-989c-4bb71ecb2108",
  "ts": "2023-06-13T09:20:02.1686644402Z"
}

Test Client> getValue Vehicle.Speed
{
  "action": "get",
  "data": {
    "dp": {
      "ts": "2023-06-13T09:20:02.500603696Z",
      "value": "54.0"
    },
    "path": "Vehicle.Speed"
  },
  "requestId": "35da4feb-a1ab-4d1c-88e0-2c23557b916c",
  "ts": "2023-06-13T09:20:09.1686644409Z"
}

Test Client> getValue Vehicle.Zpeed
{
  "action": "get",
  "error": {
    "message": "I can not find Vehicle.Zpeed in my db",
    "number": "404",
    "reason": "Path not found"
  },
  "requestId": "70491e6a-58ec-45d4-b2e2-74d37730adee",
  "ts": "2023-06-13T09:20:14.1686644414Z"
}

# Next is an example on what is returned if Server instance has been stopped

Test Client> getValue Vehicle.Speed
{
  "action": "get",
  "path": "Vehicle.Speed",
  "attribute": "value",
  "requestId": "68a26499-555d-4b09-b7eb-262fdfd65f9d",
  "error": "timeout"
}

Test Client> 
```
