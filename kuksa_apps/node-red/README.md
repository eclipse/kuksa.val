# Flow for node-red dashboard

This example uses [node-red](https://nodered.org/) but an installation is not required at your machine, as we are using Docker. You can import the json file to receive data from kuksa-val-server. Then you can use the node-red dashboard feature to view data.

## Dependencies
### Node-red extensions
The following node red extensions are used in the example flows:
- [node-red-dashboard](https://flows.nodered.org/node/node-red-dashboard)
- [node-red-contrib-web-worldmap](https://flows.nodered.org/node/node-red-contrib-web-worldmap)

### Installation
You can use docker to start `node-red` and `kuksa.val` server.
At first load `kuksa.val` docker images:
```
wget https://github.com/eclipse/kuksa.val/releases/download/0.1.8/kuksa-val-0.1.8-amd64.tar.xz
docker load -i kuksa-val-0.1.8-amd64.tar.xz
```

Or for arm64:
```
wget https://github.com/eclipse/kuksa.val/releases/download/0.1.8/kuksa-val-0.1.8-arm64.tar.xz
docker load -i kuksa-val-0.1.8-arm64.tar.xz
```

Then start all needed container using [`docker-compose.yml`](./docker-compose.yml):
```
sudo apt  install docker-compose
ARCH=arm64 docker-compose up -d
```
`-d` for detach.

By default, the mqtt flows will be configured in node-red. You can also use the import feature inside node-red to try the websocket example [below](#Websocket). 

**Note**: At first start, you may need to install the [node red extensions](#Node-red-extensions) under the [palette manager](https://nodered.org/docs/user-guide/editor/palette/manager).

Now you can view the example under [http://<node-red ip>:1880](http://localhost:1880/).
To test the example, you can use the [viss test client](../../kuksa_viss_client) or use the [gps feeder](../../kuksa_feeders/gps2val).
In [`docker-compose.yml`](./docker-compose.yml), you can find the experimental config for gps feeder container. You can comment it out to start the gps feeder directly.


## MQTT
[mqtt/flows.json](./mqtt/flows.json) subscribes some mqtt topics, which will be published by kuksa-val-server

*Note*: Do not forget to config your kuksa-val-server to publish the needed topics using the option `--mqtt.publish`.

## Websocket
- [websocket-subscription.json](./websocket-subscription.json) do the same like the mqtt example above via websocket subscription feature.
- [websocket-advanced.json](./websocket-advanced.json) implements a test client and uses secure connection with server

![screenshot](./node-red-screenshot.png)
