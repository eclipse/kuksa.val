# KUKSA.VAL
![kuksa.val Logo](./doc/pictures/logo.png)

This is KUKSA.val, the KUKSA **V**ehicle **A**bstration **L**ayer.


KUKSA.val provides in-vehicle software components for working with in-vehicle signals modelled using the [COVESA VSS data model](https://github.com/COVESA/vehicle_signal_specification).


[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) 
[![Gitter](https://badges.gitter.im/kuksa-val.svg)](https://gitter.im/kuksa-val)

[![Build kuksa-val-server](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_val_docker.yml/badge.svg)](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_val_docker.yml?query=branch%3Amaster)
[![Build kuksa-databroker](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_databroker_build.yml/badge.svg)](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_databroker_build.yml?query=branch%3Amaster)
[![codecov](https://codecov.io/gh/eclipse/kuksa.val/branch/master/graph/badge.svg?token=M4FT175771)](https://codecov.io/gh/eclipse/kuksa.val)

KUKSA.val contains several components

| Component      | Description |
| -------------- | ----------- |
| [kuksa-val-server](kuksa-val-server) | Feature rich in-vehicle data server written in C++ providing authorized access to VSS data using W3C VISS websocket protocol or GRPC       |
| [kuksa-databroker](./kuksa_databroker) | Efficient in-vehicle signal broker written in RUST
| [Python SDK](./kuksa-client)   | Python library for easy interaction with kuksa-val-server
|  [GO SDK](./kuksa_go_client)   | GOlang libraryfor easy interaction with kuksa-val-server
| [kuksa-client](./kuksa-client)   | Command line tool to interactively explore and modify the VSS data points and data structure        |
| [Examples](./kuksa_apps) | Multiple example apps for different programming languages and frameworks
| [Feeders](https://github.com/eclipse/kuksa.val.feeders/) | Multiple feeders to gathering vehicle data and transforming it to VSS suitable for kuksa-val-server



