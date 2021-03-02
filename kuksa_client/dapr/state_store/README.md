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

At first, you need to start `kuksa.val` and [testclient](../../vss-testclient) to feed data into `kuksa.val`.
You also need the dapr runtime running on your system.

You can configure `kuksa.val` and to subscribed topics using [`config.ini`](./config.ini).
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
