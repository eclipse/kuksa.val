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
# No client.authorize() as databroker does not yet support it.

response = json.loads(client.getValue('Vehicle.Speed'))
print(response['value']['value'])

client.stop()
```
