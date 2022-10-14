# Publish kuksa.val data to dapr runtime

This example utilizes a publisher and a subscriber to show, how kuksa.val works with the dapr [`pubsub`](https://docs.dapr.io/developing-applications/building-blocks/pubsub/pubsub-overview/) component.

## Pre-requisites

- [Dapr CLI and initialized environment](https://docs.dapr.io/getting-started)
- [Install Python 3.7+](https://www.python.org/downloads/)

All dependencies are listed in [requirements.txt](./requirements.txt):


```bash
pip3 install -r requirements.txt
```

## Run the example

At first, you need to start `kuksa.val` and [kuksa-client](../../../kuksa-client) to feed data into `kuksa.val`.
This dapr app `kuksavalstorage` subscribes the given vss pathes and publishes the value, if the value is changed.
To configure the subscribed vss path and the address of `kuksa.val`, you can use the config file [`config.ini`](./config.ini).

Do not forget to start the dapr runtime on your system.


```bash
dapr run --app-id python-subscriber --app-protocol grpc --app-port 50051 ./subscriber.py
dapr run --app-id python-publisher --app-protocol grpc --dapr-grpc-port=3500 ./publisher.py
```

## Cleanup

```bash
dapr stop --app-id python-publisher
dapr stop --app-id python-subscriber
```
