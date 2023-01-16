# KUKSA.val Quickstart

The quickest possible way to get KUKSA.val up and running

## Starting broker
First we want to run KUKSA.val databroker

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master 
```


## Reading and Writing VSS data via CLI
You can interact with the VSS datapoints using the cli clients. The first option is databroker-cli.

This is, how you start it:

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker-cli:master       
```

Here is how you can use it:

```
client> get Vehicle.Speed 
-> Vehicle.Speed: ( NotAvailable )
client> feed Vehicle.Speed 200
-> Ok
client> get Vehicle.Speed 
-> Vehicle.Speed: 200.00
client> 
```

An alternative is the kuksa-client CLI (based on our Python client library).

Here is how you start it:

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/kuksa-client:master --port 55555 --protocol grpc --insecure 
```

Here is how you can use it:


```
Test Client> getValue Vehicle.Speed
{
    "path": "Vehicle.Speed"
}

Test Client> setValue Vehicle.Speed 200
OK

Test Client> getValue Vehicle.Speed
{
    "path": "Vehicle.Speed",
    "value": {
        "value": 200.0,
        "timestamp": "2023-01-16T12:43:57.305350+00:00"
    }
}

Test Client> 
```

## Reading and Writing VSS data with code

Most likely, to get productive with KUKSA.val you want to interact with it programatically. Here is you you do it with our Python library.

### Generating data
Create a file `speed_provider.py` with the following content

```python
from kuksa_client.grpc import VSSClient
from kuksa_client.grpc import Datapoint

import time

with VSSClient('127.0.0.1', 55555) as client:
    for speed in range(0,100):
        client.set_current_values({
        'Vehicle.Speed': Datapoint(speed),
        })
        print(f"Setting Vehicle.Speed to {speed}")
        time.sleep(1)
print("Finished.")
```

Do a `pip install kuksa-client` and start with
python ./speed_provider.py

### Subscribing data:
Create a file `speed_subscriber.py` with the following content

```python
from kuksa_client.grpc import VSSClient

with VSSClient('127.0.0.1', 55555) as client:
    
    for updates in client.subscribe_current_values([
        'Vehicle.Speed',
    ]):
        speed = updates['Vehicle.Speed'].value
        print(f"Received updated speed: {speed}")
```

Do a `pip install kuksa-client` and start with
python ./speed_subscriber.py


## FAQ & notes
Frequently anticipated questions

### This is not working on OS X
Unfortunately OS X has a bug that does not allow you to use the databroker default 55555. To change when starting the server:

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master  --port 55556
```

Using the databroker-cli

```
docker run -it --rm --net=host -e KUKSA_DATA_BROKER_PORT=55556 ghcr.io/eclipse/kuksa.val/databroker-cli:master                   
```

Using kuksa-client CLI
```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/kuksa-client:master --port 55556 --protocol grpc --insecure 
```

### feed/set: Why is my data not updated?
Some VSS points are "senors", e.g. Vehicle.Speed. You can read/get Vehicle speed, but we are not expecting to be able to influence it via VSS.
Historically components, that gather the actual vehicle speed from some senors/busses in a vehicle and providing a VSS  reprensetation to kuksa.val have been called `feeders`. Hence, to update the current speed in the Rust-cli, you use

```
feed Vehicle.Speed 200
```

while in the Python-cli you use

```
set Vehicle.Speed 200
```

The other thing, that VSS provides you are "actuators" `Vehicle.Body.Trunk.Rear.IsOpen`. The mos important thing to remember about actors: Every actor is also a sensor, so everyting written on top applies as well!
The second-most important thing is: For VSS actors, it is expected that you might be able to influence the state of the real Vehicle by writting to them. So while being used as a sensor, you will get the current position of the Window in the exmaple, you might also want to set the _desired_ positon. 

You express this in the databroker-cli as

```
set Vehicle.Body.Trunk.Rear.IsOpen true
```

In kuksa-client cli you do

```
Test Client> setValue -a  targetValue Vehicle.Body.Trunk.Rear.IsOpen  True 
```

In the code exmaples above you would do 

```python
client.set_target_values({
        'Vehicle.Body.Trunk.Rear.IsOpen': Datapoint(True),
    })
```




### All I see is Python, shouldn't this be high-performance?
Our Python library makes it easy to interact with databroker. While this is often sufficient for many applications, you are not lmited by it: Databroker's native interface is based on GRPC, a high-perfomacne GRPC framework. GRPC enables you to generate bindings for _any_ language. Check the [GRPC website](https://grpc.io) and take a look at the [databroker interface definitions](https://github.com/eclipse/kuksa.val/tree/master/proto/kuksa/val/v1).
