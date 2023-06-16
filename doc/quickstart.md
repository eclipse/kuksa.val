# KUKSA.val Quickstart

The quickest possible way to get KUKSA.val up and running

*Note: The examples in this document do not use TLS or access control.*

## Starting broker
First we want to run KUKSA.val databroker

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master --insecure
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
client> quit
Bye bye!

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

Test Client> quit
gRPC channel disconnected.

```

## Reading and Writing VSS data with code

To realize your ideas with KUKSA.val you need to write programs that interact with its API. The easiest way to achieve this is using our Python library.

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
        print(f"Feeding Vehicle.Speed to {speed}")
        time.sleep(1)
print("Finished.")
```

Do a `pip install kuksa-client` and start with

```
python ./speed_provider.py
```

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

```
python ./speed_subscriber.py
```

## FAQ & Notes
Frequently anticipated questions and tips.

### This is not working on OS X
Unfortunately OS X has a bug that does not allow you to use the Databroker default port 55555. To change when starting the server:

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master  --port 55556 --insecure
```

Using the databroker-cli

```
docker run -it --rm --net=host -e KUKSA_DATA_BROKER_PORT=55556 ghcr.io/eclipse/kuksa.val/databroker-cli:master
```

Using kuksa-client CLI

```
docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/kuksa-client:master --port 55556 --protocol grpc --insecure 
```

### Docker desktop: Host networking not supported
The examples above all used docker's `--net=host` option. That is quite convenient for development, as basically your containers "share" your hosts networking and there is no need for any port publishing. 

However when using Docker Desktop on Mac OS or Windows, [host networking is not supported](https://docs.docker.com/network/host/).

One alternative is using a Docker distribution, that does support it even on Mac OS or Windows. [Rancher Desktop](https://rancherdesktop.io) is an alternative that does.

With Docker Desktop you can still forward ports, so this should work:

```
docker run -it --rm  --publish 55556:55556 ghcr.io/eclipse/kuksa.val/databroker:master  --port 55556 --insecure
```

From your host computer you can now reach databroker at `127.0.0.1:55556`. To connect from another container, you need to use your computers IP address (**not** 127.0.0.1), i.e. to use the client

```
docker run -it --rm  -e KUKSA_DATA_BROKER_PORT=55556 -e KUKSA_DATA_BROKER_ADDR=<YOUR_IP> ghcr.io/eclipse/kuksa.val/databroker-cli:master 
```

Recent versions of the databroker-cli also support command line arguments, so you can also write

```
docker run -it --rm   ghcr.io/eclipse/kuksa.val/databroker-cli:master  --server http://<YOUR_IP>:55556
```



### feed/set: Why is my data not updated?
Some VSS points are "sensors", e.g. Vehicle.Speed. You can read/get Vehicle speed, but we are not expecting to be able to influence it via VSS.
Historically components, that gather the actual vehicle speed from some sensors/busses in a vehicle and providing a VSS representation to kuksa.val have been called `feeders`. Hence, to update the current speed in the Rust-cli, you use

```
feed Vehicle.Speed 200
```

while in the Python-cli you use

```
set Vehicle.Speed 200
```

The other thing, that VSS provides you are "actuators" `Vehicle.Body.Trunk.Rear.IsOpen`. The most important thing to remember about actuators: Every actuators is also a sensor, so everything written on top applies as well!
The second-most important thing is: For VSS actuatorss, it is expected that you might be able to influence the state of the real Vehicle by writing to them. So while being used as a sensor, you will get the current position of the Window in the example, you might also want to set the _desired_ position.

You express this in the databroker-cli as

```
set Vehicle.Body.Trunk.Rear.IsOpen true
```

In kuksa-client cli you do

```
Test Client> setValue -a  targetValue Vehicle.Body.Trunk.Rear.IsOpen True
```

In the code examples above you would do

```python
client.set_target_values({
        'Vehicle.Body.Trunk.Rear.IsOpen': Datapoint(True),
    })
```


### All I see is Python, shouldn't this be high-performance?
Our Python library makes it easy to interact with databroker. While this is often sufficient for many applications, you are not limited by it: Databroker's native interface is based on GRPC, a high-performance GRPC framework. GRPC enables you to generate bindings for _any_ language. Check the [GRPC website](https://grpc.io) and take a look at the [databroker interface definitions](https://github.com/eclipse/kuksa.val/tree/master/proto/kuksa/val/v1).
