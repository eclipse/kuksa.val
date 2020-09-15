# MQTT dashboard via node-red

After installing [node-red](https://nodered.org/) and [node-red-dashboard](https://flows.nodered.org/node/node-red-dashboard), you can import the [node-red-examples.json](./node-red-examples.json) file to provide a nodred server, which subscribes some mqtt topics, which will published by kuksa-val-server

![screenshot](./node-red-screenshot.png)

*Note*: Do not forget to config your kuksa-val-server to publish the needed topics using the option `--mqtt.publish`.
