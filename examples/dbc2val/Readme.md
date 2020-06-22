# kuksa.val DBC feeder 

This is an early version of a DBC feeder for KUKSA.val. The basic operation is as follows

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

## ELM/OBDLink support
The feeder works best with a real CAN interface. If you use an OBD Dongle the feeder can configure it to use it as a CAN Sniffer (using  `STM` or `STMA`mode). The elmbridge will talk to the OBD Adapter using its custom AT protocol, parse the received CAN frames, and put them into a virtual CAN device. The DBC feeder can not tell the differenc

There are some limitations in this mode
 * Does not support generic ELM327 adapters but needs to have the ST commands from the OBDLink Devices available: This code has been tested with the STN2120 that is available on the KUKSA dongle, but should work on other STN chips too
 * Bandwidth/buffer overrun on Raspberry Pi internal UARTs:When connecting the STN to one of the Pis internal UARTs you will loose messages on fully loaded CAN busses (500kbit does not work when it is very loaded). The problem is not the raw bandwith (The Pi `/dev/ttyAMA0` UART can go up to 4 MBit when configured accordingly), but rather that the Pi UART IP block does not support DMA and has only an 8 bytes buffer. If the system is loaded, and you get scheduled a little too late, the overrun already occuured. While this makes this setup a little useless for a genric sniffer, in most use cases it is fine, as the code configures a dynamic whitelist according to the confgured signals, i.e. the STN is instructed to only let CAN messages containing signals of interest pass.

When using the OBD chipset, take special attention to the `obdcanack` configuration option: On a CAN bus there need to be _some_ device to acknowledge CAN frames. The STN2120 can do this. However, when tapping a vehicle bus, you probbably do not want it (as there are otehr ECUs on the bus doing it, and we want to be as passive as possible). On theother hand, on a desk setup where you have one CAN sender and the OBD chipst, you need to enable Acks, otherwise the CAN sender will go into error mode, if no acknowledgement is received. 

## Configuration
Look at config.ini and mapping.yml to figure it out. The example is configured for a Tesla Model 3 using the DBC found here https://github.com/joshwardell/model3dbc 


## Limitations
 * currently only works with VSS server in `--insecure` mode
 * No data conversions supported
