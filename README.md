# KUKSA.VAL
![kuksa.val Logo](./doc/pictures/logo.png)

This is KUKSA.val, the KUKSA **V**ehicle **A**bstration **L**ayer.


KUKSA.val provides in-vehicle software components for working with in-vehicle signals modelled using the [COVESA VSS data model](https://github.com/COVESA/vehicle_signal_specification). 



[![Gitter](https://badges.gitter.im/kuksa-val.svg)](https://gitter.im/kuksa-val)

![Build kuksa-val-server](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_val_docker.yml/badge.svg)

[![Build Status](https://kuksaval.northeurope.cloudapp.azure.com/buildStatus/icon?job=kuksaval-upstream%2Fmaster&subject=kuksa-val-server%20legacy%20CI)](https://kuksaval.northeurope.cloudapp.azure.com/job/kuksaval-upstream/job/master/)
[![codecov](https://codecov.io/gh/eclipse/kuksa.val/branch/master/graph/badge.svg?token=M4FT175771)](https://codecov.io/gh/eclipse/kuksa.val)

This repository contains several components 

| License | Component      | Description |
| --------| -------------- | ----------- |
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [kuksa-val-server](kuksa-val-server) | Feature rich in-vehicle data server written in C++ providing authorized access to VSS data using W3C VISS websocket protocol or GRPC       |
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [Python SDK](./kuksa_viss_client)   | Python library for easy interaction with kuksa-val-server  
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [GO SDK](./kuksa_go_client)   | GOlang libraryfor easy interaction with kuksa-val-server  
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [Testclient](./kuksa_viss_client)   | Command line tool to interactively explore and modify the VSS data points and data structure        |
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [Examples](./kuksa_apps) | Multiple example apps for different programming languages and frameworks
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [Feeders](./kuksa-feeders) | Multiple feeders to gathering vehicle data and transforming it to VSS suitable for kuksa-val-server
| [![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0) | [kuksa-databroker](./kuksa_databroker) | Efficient in-vehicle signal broker written in RUST



