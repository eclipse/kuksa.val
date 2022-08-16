# Publish kuksa.val data via Eclipse Kanto
The provided implementation is a bridge between kuksa.val and Eclipse Kanto
that enables VSS data to be handled between these components. It also
utilizes the deployment of kuksa.val and the respective data feeders
via the OTA container management capabilities provided by Eclipse Kanto at the edge.

Check [config.ini](config/edge_client.ini) file for the configuration of this client.


```
             +----------------+
             |                |
             |     Eclipse    |
             |      Ditto     |
             |                |
             +-----+-----+----+
                   |MQTT |
                   +--^--+
                      |
                      |
+---------------------+--------------------------+
| Kuksa-Kanto-Bridge  |                          |
|                     |                          |
|     +---------------+---------------------+    |
|     | Publish data via Ditto-MQTT-Client  |    |
|     |    with the use of Eclipse Paho     |    |
|     +---------------^---------------------+    |
|                     |                          |
|           +---------+------------+             |
|           |                      |             |
|           |                      |             |
|           |   Kuksa VISS Client  |             |
|           |                      |             |
|           |                      |             |
|           +----------+-----------+             |
|                      |                         |
+----------------------+-------------------------+
                       |Subscribe to VSS-paths
                       +----------------+
                                        |
+-----------------------+    +----------v-----------+
|                       |    |                      |
|                       |    |                      |
|     Data feeders      |    |   Kuksa.val server   |
|         like          +---->      VSS-Server      |
|      DBC or GPS       |    |                      |
|                       |    |                      |
|                       |    |                      |
+-----------------------+    +----------------------+
```




# Prerequisites
* [Python 3.7+](https://www.python.org/downloads/)
* [Docker](https://docs.docker.com/get-docker/)

All dependencies are listed in requirements.txt:
```bash
pip3 install -r requirements.txt
```
To avoid conflicts, a Python virtual environment can be utilized.


# Run the example

## Install dependencies and execution

```bash
pip3 install -r requirements.txt
python edge_client.py
```

## Using Docker
You can also use `docker` to build and execute the application.

```bash
docker build -t kuksa_kanto_bridge .
```

To run:

```bash
docker run -it kuksa_kanto_bridge
```

The application can be used by building the Docker image and push it to a private or local registry. 
You may use Eclipse Kanto to pull and start the container from the registry.
The Kuksa-Kanto-Bridge subscribes the given VSS paths and publishes the value, if the value is changed.

To configure the subscribed vss paths and the address of kuksa.val and Eclipse Paho, you can use the config file
[config.ini](config/edge_client.ini).

