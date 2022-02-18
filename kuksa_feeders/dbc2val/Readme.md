# kuksa.val DBC feeder 

This is a DBC feeder for KUKSA.val. The basic operation is as follows

``` 
                             +-------------+                                              
                             |   DBCFile   |                                              
                             +-------------+                                              
                                    |                                                     
                                    |                                                     
+-----------------+         +-------|------+          +-----------------+                 
|                 |         |              |          |                 |                 
|  CAN Interface  |------- >|  DBCFeeder   | -------->|   VSS Server    |                 
|                 |         |              |          |                 |                -
+-----------------+         +--------------+          +-----------------+                 
                                    |                                                     
                                    |                                                     
                             +-------------+                                              
                             | mapping.yml |                                              
                             +-------------+                                                                                                                               
```

The feeder connects to a socket CAN interface and reads raw CAN data, that will be parsed based on a DBC file. The mapping file describes which DBC signal should be amtched to which part in the VSS tree.

## Install and run the feeder
Install requirements with
```
pip install -r requirements.txt
```

The DBC feeder is configured using a config file. Check [dbc_feeder.ini](config/dbc_feeder.ini) for supported options.


## Example mapping and data
KUKSA.val includes an example can trace to play around. The file `candump-2021-12-08_151848.log.xz`
includes a CAN trace from  2018 Tesla M3 with software 2021.40.6.
This data is interpreted using the [Model3CAN.dbc](./Model3CAN.dbc) [maintained by Josh Wardell](https://github.com/joshwardell/model3dbc).

The canlog in the repo is compressed, to uncompress it (will be around 150MB) do 
```
unxz candump-2021-12-08_151848.log.xz
```

The mapping.yml describes how to map CAN data described in a DBC file to VSS. Let's look at some example entries

```yaml
PCS_dcdcLvBusVolt:
  minupdatedelay: 1000
  targets:
    Vehicle.OBD.ControlModuleVoltage: {}
```
This takes the signal `PCS_dcdcLvBusVolt` from the DBC file and updates VSS path `Vehicle.OBD.ControlModuleVoltage`. Except the scaling/mapping described in the DBC file, no further preprocessing takes place before writing the received value to KUKSA.val. 

`minupdatedelay` specifies a minimum time in ms that needs to pass before a signal will be updated in KUKSA.val again. This is useful for high frequency signals to lessen the load on the system, e.g. if a battery voltage is put on the CAN every 5 ms, it is unlikely we need to update it as often.

A more complex example:

```yaml
VCFRONT_brakeFluidLevel:
  minupdatedelay: 1000
  targets:
    Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.FluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
    Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.FluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
```
Here the same DBC signal `VCFRONT_brakeFluidLevel`is mapped to different VSS path. Also _fullmapping_ transform is applied. The value `LOW` from the DBC is mapped to the value `true` in the VSS path and `NORMAL`is mapped to `false`. In the _fullmapping_ transform, if no match is found, the value will be ignored. There also exists a _partialmapping_ transform, which works similarly, with the difference, that if no match is found the value from the DBC will be written as-is to KUKSA.val.

Another transform is the _math_ transform

```yaml
VCLEFT_mirrorTiltXPosition:
   minupdatedelay: 100
   targets:
    Vehicle.Body.Mirrors.Left.Pan:
      transform: #scale 0..5 to -100..100
        math: floor((x*40)-100)
```

This can be used if the scale of a signal as described in the DBC is not compatible with the VSS model.  The value - with all transforms described in the DBC -  is used as `x` on the formula given by the _math_ transform. For available operators, functions and constants supported by the _math_ transform check https://pypi.org/project/py-expression-eval/.


Take a look at the mapping.yml to see more examples.

## Testing using virtual can bus
You can use the following command to create an virtual CAN interface and replay recorded candump log to test the dbc feeder:

```
./createvcan.sh vcan0
canplayer vcan0=can0 -v -I candump-2021-12-08_151848.log 
```

## ELM/OBDLink support
The feeder works best with a real CAN interface. If you use an OBD Dongle the feeder can configure it to use it as a CAN Sniffer (using  `STM` or `STMA` mode). The elmbridge will talk to the OBD Adapter using its custom AT protocol, parse the received CAN frames, and put them into a virtual CAN device. The DBC feeder can not tell the differenc

There are some limitations in this mode
 * Does not support generic ELM327 adapters but needs to have the ST commands from the OBDLink Devices available: This code has been tested with the STN2120 that is available on the KUKSA dongle, but should work on other STN chips too
 * Bandwidth/buffer overrun on Raspberry Pi internal UARTs:When connecting the STN to one of the Pis internal UARTs you will loose messages on fully loaded CAN busses (500kbit does not work when it is very loaded). The problem is not the raw bandwith (The Pi `/dev/ttyAMA0` UART can go up to 4 MBit when configured accordingly), but rather that the Pi UART IP block does not support DMA and has only an 8 bytes buffer. If the system is loaded, and you get scheduled a little too late, the overrun already occuured. While this makes this setup a little useless for a generic sniffer, in most use cases it is fine, as the code configures a dynamic whitelist according to the confgured signals, i.e. the STN is instructed to only let CAN messages containing signals of interest pass.

When using the OBD chipset, take special attention to the `obdcanack` configuration option: On a CAN bus there needs to be _some_ device to acknowledge CAN frames. The STN2120 can do this. However, when tapping a vehicle bus, you probbably do not want it (as there are otehr ECUs on the bus doing it, and we want to be as passive as possible). On theother hand, on a desk setup where you have one CAN sender and the OBD chipst, you need to enable Acks, otherwise the CAN sender will go into error mode, if no acknowledgement is received. 

## SAE-J1939 support
When the target DBC file and ECU follow the SAE-J1939 standard, the CAN reader application of the feeder should read PGN(Parameter Group Number)-based Data rather than CAN frames directly. Otherwise it is possible to miss signals from large-sized messages that are delivered with more than one CAN frame because the size of each of these messages is bigger than a CAN frame's maximum payload of 8 bytes. To enable the J1939 mode, simply put `--j1939` in the command when running `dbcfeeder.py`.
Prior to using this feature, j1939 and the relevant wheel-packages should be installed first:

```
pip install j1939
git clone https://github.com/benkfra/j1939.git
cd j1939
pip install .
```

The detailed documentation to this feature can be found here https://dias-kuksa-doc.readthedocs.io/en/latest/contents/j1939.html




