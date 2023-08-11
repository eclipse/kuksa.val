# KUKSA.VAL
![kuksa.val Logo](./doc/pictures/logo.png)

This is KUKSA.val, the KUKSA **V**ehicle **A**bstraction **L**ayer.


KUKSA.val provides in-vehicle software components for working with in-vehicle signals modelled using the [COVESA VSS data model](https://github.com/COVESA/vehicle_signal_specification).

If you are new here, try the [Quickstart](doc/quickstart.md), which should not take more than 10 min of your time.


[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0)
[![Gitter](https://img.shields.io/gitter/room/kuksa-val/community)](https://gitter.im/kuksa-val/community)

[![Build kuksa-val-server](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_val_docker.yml/badge.svg)](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_val_docker.yml?query=branch%3Amaster)
[![Build kuksa-databroker](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_databroker_build.yml/badge.svg)](https://github.com/eclipse/kuksa.val/actions/workflows/kuksa_databroker_build.yml?query=branch%3Amaster)
[![codecov](https://codecov.io/gh/eclipse/kuksa.val/branch/master/graph/badge.svg?token=M4FT175771)](https://codecov.io/gh/eclipse/kuksa.val)

KUKSA.val contains several components

| Component      | Description |
| -------------- | ----------- |
| [KUKSA Databroker](./kuksa_databroker) | Efficient in-vehicle signal broker written in RUST providing authorized access to VSS data using gRPC
| [KUKSA Server](kuksa-val-server) | Feature rich in-vehicle data server written in C++ providing authorized access to VSS data using W3C VISS websocket protocol
| [KUKSA Python Client and SDK](./kuksa-client)   | Command line tool to interactively explore and modify the VSS data points and data structure. Python library for easy interaction with KUKSA Databroker and Server
| [KUKSA GO Client](./kuksa_go_client)   | Example client written in the [GO](https://go.dev/) programming language for easy interaction with KUKSA Databroker and Server
| [Example Applications](./kuksa_apps) | Some example apps for different programming languages and frameworks
| [Feeders and Providers](https://github.com/eclipse/kuksa.val.feeders/) | Multiple feeders and providers for exchanging vehicle data with KUKSA databroker and Server

## More information

* [KUKSA.val TLS Concept](doc/tls.md)

## Pre-commit set up
This repository is set up to use [pre-commit](https://pre-commit.com/) hooks.
Use `pip install pre-commit` to install pre-commit.
After you clone the project, run `pre-commit install` to install pre-commit into your git hooks.
Pre-commit will now run on every commit.
Every time you clone a project using pre-commit running pre-commit install should always be the first thing you do.
