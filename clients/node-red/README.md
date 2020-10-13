# Flow for node-red dashboard

After installing [node-red](https://nodered.org/) and [node-red-dashboard](https://flows.nodered.org/node/node-red-dashboard), you can import the json file to provide a node-red server. Then you can use the dashboard feature to view data from kuksa-val-server.

## MQTT
[mqtt-examples.json](./mqtt-examples.json) subscribes some mqtt topics, which will published by kuksa-val-server

![screenshot](./node-red-screenshot.png)
*Note*: Do not forget to config your kuksa-val-server to publish the needed topics using the option `--mqtt.publish`.

## Websocket
- [websocket-subscription.json](./websocket-subscription.json) do the same like the mqtt example above via websocket subscription feature.
- [websocket-advanced.json](./websocket-advanced.json) implements a test client and uses secure connection with server

### TODO
It is also possible to use node-red as websocket client, it is also configured in the json file, but it does not work jet
