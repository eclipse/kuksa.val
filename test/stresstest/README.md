The configuration for the stresstest client and the instance of the KUKSA.val server or databroker is configured by a config file like [config.ini](config.ini). By default this file is expected to be in the same directory as [stresstest.py](client.py), [broker.py](broker.py) or [server.py](server.py). If you want to use a config.ini from another directory you could specify your file by adding the command line argument 
``` 
--file 
``` 
or 
``` 
-f 
``` 
to the invoke of the scripts.
In the configuration file you can specify:
* port = port which gets used to connect the stress clients
* ip = ip address which gets used to connect the stress clients
* protocol = if the client should call websocket (ws, only supported by server) or gRPC's (grpc)
* insecure = Dis/Allows insecure connections to KUKSA.val server/databroker (False/True)
* subscriberCount = amount of subscriber you want to spawn (0 is also allowed)
* path = which VSS path gets used to stress
* Debug = Display debug messages(1) else (0)

Before starting the stresstest run either: 
``` 
python3 broker.py 
``` 
or 
``` 
python3 server.py
```
Make sure it is fully started before you start the stresstest.
The stresstest can be invoked by running: 
```
python3 stresstest.py 
```


The stresstest stresses setValue at the moment and builds everything from scratch. Alternatively we should be able to pull docker images and run them and stresstest these instances. The second stresstest that could be performed is subscribing with a certain amount of subcribers. This checks how the bandwidth and everything changes with 100 or more subscribers.