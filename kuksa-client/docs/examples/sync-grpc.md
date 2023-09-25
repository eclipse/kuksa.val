# Synchronous API

`kuksa_client.grpc.VSSClient` provides a synchronous client that only supports `grpc` to interact with `kuksa_databroker`.
'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'

## Usage

This API is split in 2 sets of functions:
- [Simplified API](#simplified-api) to quickly get started getting/setting/subscribing to current values, target values or metadata
- [Full-fledged API](#full-fledged-api) to achieve more complex use cases and get extra performance (e.g. limit the number of RPCs)

### Simplified API

This API makes the assumption you're interested in getting/setting/subscribing to either:

- current values i.e. sensor, attribute values
- or target values i.e. actuator target values
- or metadata

Hence this is the go-to API if you're seeking minimum boilerplate and the most basic functionality.

#### Examples not leveraging authorization

Here's the simplest example how one can retrieve vehicle's current speed using the synchronous API:
```python
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    current_values = client.get_current_values([
        'Vehicle.Speed',
    ])
     if current_values['Vehicle.Speed'] is not None:
        print(current_values['Vehicle.Speed'].value)
```

Besides this there is a solution where you are not using the client as context-manager.
Then you must explicitly call `connect()`.

```python
from kuksa_client.grpc import VSSClient

client = VSSClient('127.0.0.1', 55555)
client.connect()
current_values = client.get_current_values([
    'Vehicle.Speed',
])
if current_values['Vehicle.Speed'] is not None:
    print(current_values['Vehicle.Speed'].value)
```

Here's another example how one can actuate a wiping system:
```python
from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    client.set_target_values({
        'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition': Datapoint(45),
    })
```

Here's an example how one can actuate a wiping system and monitor the position:
```python
from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    client.set_target_values({
        'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition': Datapoint(45),
    })

    for updates in client.subscribe_current_values([
        'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition',
    ]):
        if updates['Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'] is not None:
            current_position = updates['Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'].value
            print(f"Current wiper position is: {current_position}")
```

#### Examples leveraging authorization
Here's an example how to use authorization with kuksa-client
```python
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    # token from kuksa.val/jwt/provide-all.json
    token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZSJ9.OJWzTvDjcmeWyg3vmBR5TEtqYaHq8HrpFLlTKZAfDBAQBUHpyUEboJ97jfWuWgBnTpnfboyfAbwvLqo6bEVZ6tXzF8n9LtW6HmPbIWoDqXuobM2grUCVaGKuOcnCpMCQYChziqHbYwRJYP9nkYgbQU1kE4dN7880Io4xzq0GEbWksB2CVpOoExQUmCZpCohPs-XEkdmXhcUKnWnOeiSsRGKusx987vpY_WOXh6WE7DfJgzAgpPDo33qI7zQuTzUILORQsiHmsrQO0-zcvokNjaQUzlt5ETZ7MQLCtiUQaN0NMbDMCWkmSfNvZ5hKCNbfr2FaiMzrGBOQdvQiFo-DqZKGNweaGpufYXuaKfn3SXKoDr8u1xDE5oKgWMjxDR9pQYGzIF5bDXITSywCm4kN5DIn7e2_Ga28h3rBl0t0ZT0cwlszftQRueDTFcMns1u9PEDOqf7fRrhjq3zqpxuMAoRANVd2z237eBsS0AvdSIxL52N4xO8P_h93NN8Vaum28fTPxzm8p9WlQh4mgUelggtT415hLcxizx15ARIRG0RiW91Pglzt4WRtXHnsg93Ixd3yXXzZ2i4Y0hqhj_L12SsXunK2VxKup2sFCQz6wM-t_7ADmNYcs80idzsadY8rYKDV8N1WqOOd4ANG_nzWa86Tyu6wAwhDVag5nbFmLZQ"
    client.authorize(token)
    current_values = client.get_current_values([
        'Vehicle.Speed',
    ])
    if current_values['Vehicle.Speed'] is not None:
        print(current_values['Vehicle.Speed'].value)
```

It is also possible to give the token to the client during initialization.
```python
from kuksa_client.grpc.aio import VSSClient

# token from kuksa.val/jwt/provide-all.json
_token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJsb2NhbCBkZXYiLCJpc3MiOiJjcmVhdGVUb2tlbi5weSIsImF1ZCI6WyJrdWtzYS52YWwiXSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE3NjcyMjU1OTksInNjb3BlIjoicHJvdmlkZSJ9.OJWzTvDjcmeWyg3vmBR5TEtqYaHq8HrpFLlTKZAfDBAQBUHpyUEboJ97jfWuWgBnTpnfboyfAbwvLqo6bEVZ6tXzF8n9LtW6HmPbIWoDqXuobM2grUCVaGKuOcnCpMCQYChziqHbYwRJYP9nkYgbQU1kE4dN7880Io4xzq0GEbWksB2CVpOoExQUmCZpCohPs-XEkdmXhcUKnWnOeiSsRGKusx987vpY_WOXh6WE7DfJgzAgpPDo33qI7zQuTzUILORQsiHmsrQO0-zcvokNjaQUzlt5ETZ7MQLCtiUQaN0NMbDMCWkmSfNvZ5hKCNbfr2FaiMzrGBOQdvQiFo-DqZKGNweaGpufYXuaKfn3SXKoDr8u1xDE5oKgWMjxDR9pQYGzIF5bDXITSywCm4kN5DIn7e2_Ga28h3rBl0t0ZT0cwlszftQRueDTFcMns1u9PEDOqf7fRrhjq3zqpxuMAoRANVd2z237eBsS0AvdSIxL52N4xO8P_h93NN8Vaum28fTPxzm8p9WlQh4mgUelggtT415hLcxizx15ARIRG0RiW91Pglzt4WRtXHnsg93Ixd3yXXzZ2i4Y0hqhj_L12SsXunK2VxKup2sFCQz6wM-t_7ADmNYcs80idzsadY8rYKDV8N1WqOOd4ANG_nzWa86Tyu6wAwhDVag5nbFmLZQ"
with VSSClient('127.0.0.1', 55555, token=_token) as client:
    current_values = await client.get_current_values([
        'Vehicle.Speed',
    ])
    if current_values['Vehicle.Speed'] is not None:
        print(current_values['Vehicle.Speed'].value)
```

### Full-fledged API

#### Get current value, target value and metadata for a single entry in a single RPC

```python
from kuksa_client.grpc import EntryRequest
from kuksa_client.grpc import Field
from kuksa_client.grpc import View
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    path = 'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'
    entries = client.get(entries=[
        EntryRequest(path, View.ALL, (Field.UNSPECIFIED,)),
    ])
    if entries[0] is not None:
        print(f"{path} current value: {entries[0].value}")
        print(f"{path} target value: {entries[0].actuator_target}")
        print(f"{path} metadata: {entries[0].metadata}")
```

#### Simple vehicle speed feeder

Suppose you want to periodically decode vehicle speed from the CAN bus and send it to the server.
With the simplified API, you would just periodically call `set_current_values({'Vehicle.Speed': Datapoint(new_speed)})`.
However as the client has no knowledge about `Vehicle.Speed`'s data type, it will query `Vehicle.Speed`'s metadata
before each and every call to `set()`.
With the full-fledged API, one can provide the data type to be used and remove the need for getting metadata:

```python
import random
import time

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import DataType
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import Metadata
from kuksa_client.grpc import VSSClient

def read_speed_from_can_bus():
    """Stub of a real function that would read vehicle speed from CAN bus."""
    time.sleep(0.1)
    return random.randrange(250)

with VSSClient('127.0.0.1', 55555) as client:
    while True:
        new_speed = read_speed_from_can_bus()
        updates = (EntryUpdate(DataEntry(
            'Vehicle.Speed',
            value=Datapoint(value=new_speed),
            metadata=Metadata(data_type=DataType.FLOAT),
        ), (Field.VALUE,)),)
        client.set(updates=updates)
```

#### Subscribe to current and target values updates for multiple entries

```python
from kuksa_client.grpc import Field
from kuksa_client.grpc import SubscribeEntry
from kuksa_client.grpc import View
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    for updates in client.subscribe(entries=[
        SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.Frequency', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
        SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.Mode', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
        SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
    ]):
        for update in updates:
            if update.entry.value is not None:
                print(f"Current value for {update.entry.path} is now: {update.entry.value}")
            if update.entry.actuator_target is not None:
                print(f"Target value for {update.entry.path} is now: {update.entry.actuator_target}")
```
