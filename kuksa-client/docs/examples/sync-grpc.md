# Synchronous API

`kuksa_client.grpc.VSSClient` provides a synchronous client that only supports `grpc` to interact with `kuksa_databroker`.
'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'

> **Note**
> The synchronous API does not yet support subscribing to values/target values/metadata changes.

## Usage

This API is split in 2 sets of functions:
- [Simplified API](#simplified-api) to quickly get started getting/setting current values, target values or metadata
- [Full-fledged API](#full-fledged-api) to achieve more complex use cases and get extra performance (e.g. limit the number of RPCs)

### Simplified API

This API makes the assumption you're interested in getting/setting either:

- current values i.e. sensor, attribute values
- or target values i.e. actuator target values
- or metadata

Hence this is the go-to API if you're seeking minimum boilerplate and the most basic functionality.

Here's the simplest example how one can retrieve vehicle's current speed using the synchronous API:
```python
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    current_values = client.get_current_values([
        'Vehicle.Speed',
    ])
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
