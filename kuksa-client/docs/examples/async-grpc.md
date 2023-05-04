# Asynchronous API (asyncio)

`kuksa_client.grpc.aio.VSSClient` provides an asynchronous client that only supports `grpc` to interact with `kuksa_databroker`.

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

Here's the simplest example how one can retrieve vehicle's current speed using the asynchronous API:
```python
import asyncio

from kuksa_client.grpc.aio import VSSClient

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        current_values = await client.get_current_values([
            'Vehicle.Speed',
        ])
        print(current_values['Vehicle.Speed'].value)

asyncio.run(main())
```

Besides this there is a solution where you are not using the client as context-manager
```python
import asyncio

from kuksa_client.grpc.aio import VSSClient

async def main():
    client = VSSClient('127.0.0.1', 55555):
    current_values = await client.get_current_values([
        'Vehicle.Speed',
    ])
    print(current_values['Vehicle.Speed'].value)

asyncio.run(main())
```

Here's another example how one can actuate a wiping system:
```python
import asyncio

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc.aio import VSSClient

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        await client.set_target_values({
            'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition': Datapoint(45),
        })

asyncio.run(main())
```

Here's an example how one can actuate a wiping system and monitor the position:
```python
import asyncio

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc.aio import VSSClient

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        await client.set_target_values({
            'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition': Datapoint(45),
        })

        async for updates in client.subscribe_current_values([
            'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition',
        ]):
            current_position = updates['Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'].value
            print(f"Current wiper position is: {current_position}")


asyncio.run(main())
```

### Full-fledged API

#### Get current value, target value and metadata for a single entry in a single RPC

```python
import asyncio

from kuksa_client.grpc import EntryRequest
from kuksa_client.grpc import Field
from kuksa_client.grpc import View
from kuksa_client.grpc.aio import VSSClient

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        path = 'Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition'
        entries = await client.get(entries=[
            EntryRequest(path, View.ALL, (Field.UNSPECIFIED,)),
        ])
        print(f"{path} current value: {entries[0].value}")
        print(f"{path} target value: {entries[0].actuator_target}")
        print(f"{path} metadata: {entries[0].metadata}")

asyncio.run(main())
```

#### Simple vehicle speed feeder

Suppose you want to periodically decode vehicle speed from the CAN bus and send it to the server.
With the simplified API, you would just periodically call `set_current_values({'Vehicle.Speed': Datapoint(new_speed)})`.
However as the client has no knowledge about `Vehicle.Speed`'s data type, it will query `Vehicle.Speed`'s metadata
before each and every call to `set()`.
With the full-fledged API, one can provide the data type to be used and remove the need for getting metadata:

```python
import asyncio
import random

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import DataType
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import Metadata
from kuksa_client.grpc.aio import VSSClient

async def read_speed_from_can_bus():
    """Stub of a real coroutine that would read vehicle speed from CAN bus."""
    await asyncio.sleep(0.1)
    return random.randrange(250)

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        while True:
            new_speed = await read_speed_from_can_bus()
            updates = (EntryUpdate(DataEntry(
                'Vehicle.Speed',
                value=Datapoint(value=new_speed),
                metadata=Metadata(data_type=DataType.FLOAT),
            ), (Field.VALUE,)),)
            await client.set(updates=updates)

asyncio.run(main())
```

#### Subscribe to current and target values updates for multiple entries


```python
import asyncio

from kuksa_client.grpc import Field
from kuksa_client.grpc import SubscribeEntry
from kuksa_client.grpc import View
from kuksa_client.grpc.aio import VSSClient

async def main():
    async with VSSClient('127.0.0.1', 55555) as client:
        async for updates in client.subscribe(entries=[
            SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.Frequency', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
            SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.Mode', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
            SubscribeEntry('Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition', View.FIELDS, (Field.VALUE, Field.ACTUATOR_TARGET)),
        ]):
            for update in updates:
                if update.entry.value is not None:
                    print(f"Current value for {update.entry.path} is now: {update.entry.value}")
                if update.entry.actuator_target is not None:
                    print(f"Target value for {update.entry.path} is now: {update.entry.actuator_target}")

asyncio.run(main())
```
