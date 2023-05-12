# KUKSA.val VSS handling

## Introduction

KUKSA.val is adapted to use Vehicle Signals Specification as defined by COVESA.
The ambition is to always support the latest released version available at the
[COVESA VSS release page](https://github.com/COVESA/vehicle_signal_specification/releases).
In addition older versions may be supported. This folder contains copies of all versions supported.

## Supported VSS versions

* [VSS 3.1.1](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v3.1.1)
* [VSS 3.0](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v3.0)
* [VSS 2.2](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v2.2)
* [VSS 2.1](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v2.1)
* [VSS 2.0](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v2.0)

### Known limitations

* [VSS 3.1](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v3.1) is not supported as it contains
  a branch without description. Descriptions are mandatory in VSS but that is currently not checked by vss-tools.
  However, KUKSA.val databroker requires description to be present.
  Use [VSS 3.1.1](https://github.com/COVESA/vehicle_signal_specification/releases/tag/v3.1.1) instead.

## Change process

This is the process for introducing support for a new VSS version:

* Copy the new json file to this folder, note that VSS releases use a a slightly different naming convention,
  adapt the name to the pattern used in KUKSA.val
* Check if KUKSA.val code relying on VSS syntax needs to be updated to manage changes in syntax
* Check if examples needs to be updated due to changed signal names or syntax
* Change build scripts and examples to use the new version as default
    * Search for the old version number and replace where needed
* If needed, adapt or extend test cases to use the new version instead of previous version
* Remember to also integrate new version in [KUKSA Feeder](https://github.com/eclipse/kuksa.val.feeders) repository
    * Needed at least for dbc2val

## Tests after update

### Kuksa-val-server unit tests
* Run kuksa-val-server unit tests according to [documentation](../../kuksa-val-server/test/unit-test/readme.md)

### Kuksa-val-server smoke test
* Build and start kuksa-val-server with new VSS release as described in the [README](https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-server/README.md)
* If needed [generate new certificates](https://github.com/eclipse/kuksa.val/tree/master/kuksa_certificates)
* [Start Kuksa Client](https://github.com/eclipse/kuksa.val/blob/master/kuksa-client/README.md) and perform some basic tests that VSS changes are present

Examples:

Start and authorize with generated token:

```
$ python -m kuksa_client
Welcome to Kuksa Client version 0.2.1

                  `-:+o/shhhs+:`
                ./oo/+o/``.-:ohhs-
              `/o+-  /o/  `..  :yho`
              +o/    /o/  oho    ohy`
             :o+     /o/`+hh.     sh+
             +o:     /oo+o+`      /hy
             +o:     /o+/oo-      +hs
             .oo`    oho `oo-    .hh:
              :oo.   oho  -+:   -hh/
               .+o+-`oho     `:shy-
                 ./o/ohy//+oyhho-
                    `-/+oo+/:.

Default tokens directory: /home/erik/.local/lib/python3.9/site-packages/kuksa_certificates/jwt

connect to wss://127.0.0.1:8090
Websocket connected!!

Test Client> authorize /home/user/.local/lib/python3.9/site-packages/kuksa_certificates/jwt/super-admin.json.token

```

Check that changes to the new version has taken effect, e.g. that new signals exist or that changed or added attributes are present.

```
Test Client> getMetaData Vehicle.Powertrain.Transmission.DriveType

```

Do some set/get on new signals to verify that it works as expected.

```
Test Client> setValue Vehicle.CurrentLocation.Longitude 16.346734

Test Client> getValue Vehicle.CurrentLocation.Longitude

```

### Kuksa-val-server and dbc2val smoke test

Run dbc2val as described in [documentation](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/Readme.md) using example [dump file](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/candump.log). Verify that no errors appear in kuksa-val-server log. Not all signals in the [mapping files](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/mapping/) are used by the example dump file, but it can be verified using Kuksa Client that e.g. `Vehicle.Speed` has been given a value.


### Kuksa-val-server and gps2val smoke test

Run gps2val as described in [documentation](https://github.com/eclipse/kuksa.val.feeders/blob/main/gps2val/readme.md) using example log. If gpsd already is running at port 2947 a different port like 2949 may be used. Then [gps2val config file](https://github.com/eclipse/kuksa.val.feeders/blob/main/gps2val/config/gpsd_feeder.ini) must also be updated to use that port. If device handling does not work on test platform `-t` can be used to uss TCP instead.

```
gpsfake -t -P 2949 simplelog_example.nmea
```

If everything works as expected successful set requests with values similar to below shall be seen:

```
VERBOSE: SubscriptionHandler::publishForVSSPath: set value 48.811483333 for path Vehicle.CurrentLocation.Latitude
...
VERBOSE: SubscriptionHandler::publishForVSSPath: set value 8.927633333 for path Vehicle.CurrentLocation.Longitude
...
VERBOSE: SubscriptionHandler::publishForVSSPath: set value 12.244000434875488 for path Vehicle.Speed
```

The [Kuksa Client](https://github.com/eclipse/kuksa.val/blob/master/kuksa-client/README.md) can be used to verify that data actually is correctly interpreted.

```
Test Client> getValue Vehicle.CurrentLocation.Latitude
{"action":"get","data":{"dp":{"ts":"2022-08-19T14:04:43.339202324Z","value":"48.780133333"},"path":"Vehicle.CurrentLocation.Latitude"},"requestId":"025e5b10-d389-4a96-ba43-46f6a8c06b9e","ts":"2022-08-19T14:04:43.1660914283Z"}
```


### Kuksa_databroker smoke test

Build and run kuksa_databroker using the new VSS file according to [documentation](../../kuksa_databroker/README.md), e.g.

```sh
$cargo run --bin databroker -- --metadata ../data/vss-core/vss_release_3.1.1.json
```

Use the client to verify that changes in VSS are reflected, by doing e.g. set/get on some new or renamed signals.

```sh
$cargo run --bin databroker-cli

client> set Vehicle.CurrentLocation.Latitude 3
-> Ok
client> get Vehicle.CurrentLocation.Latitude
-> Vehicle.CurrentLocation.Latitude: 3
```

### Kuksa_databroker and dbc2val smoke test

Run dbc2val as described in [documentation](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/Readme.md) using example [dump file](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/candump.log). It is important to use databroker mode.

```sh
./dbcfeeder.py --usecase databroker --address=127.0.0.1:55555
```
Verify that no errors appear in kuksa-val-server log. Not all signals in the [mapping files](https://github.com/eclipse/kuksa.val.feeders/blob/main/dbc2val/mapping) are used by the example dump file, but it can be verified using Kuksa Client that e.g. `Vehicle.Speed` has been given a value.

