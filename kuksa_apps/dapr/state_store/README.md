# Store kuksa.val data to dapr runtime

This example shows, how kuksa.val works with the dapr state management component [`statestore`](https://docs.dapr.io/developing-applications/building-blocks/state-management/state-management-overview/).

## Pre-requisites

- [Dapr CLI and initialized environment](https://docs.dapr.io/getting-started)
- [Install Python 3.7+](https://www.python.org/downloads/)

All dependencies are listed in [requirements.txt](./requirements.txt):


```bash
pip3 install -r requirements.txt
```

## Run the example

At first, you need to start `kuksa.val` and [kuksa viss client](../../../kuksa_viss_client) to feed data into `kuksa.val`.
This dapr app `kuksavalstorage` subscribes the given vss pathes and stores the value, if the value is changed.
To configure the subscribed vss path and the address of `kuksa.val`, you can use the config file [`config.ini`](./config.ini).

Do not forget to start the dapr runtime on your system.

Then run the following commands to start the dapr application:

```bash
dapr run --app-id kuksavalstorage --dapr-http-port 3501 ./state_store.py

```

After that, you can read kuksa data using dapr supported interface and tooling. For example:

```
curl http://localhost:3501/v1.0/state/statestore/Vehicle.Speed
```

will return the current value of `Vehicle.Speed`.

## Cleanup

```bash
dapr stop --app-id kuksavalstorage
```
