# System Architecture and Deployment

The KUKSA.val server aims to provide a consistent view of all signals inside a vehicle. The data model follows the [COVESA Vehicle Signal Specification](https://github.com/COVESA/vehicle_signal_specification) data is accessed using a variant of the [W3C VISS protocol](https://github.com/w3c/automotive).

The following picture shows the basic system architecture

![Basic architecture](./pictures/sysarch_basic.svg)

The KUKSA.val server runs on a vehicle computer with a given VSS model, the model is populated and updated by `data feeder` components. A data feeder might get its infomation directly fom a sensor, from the vehicle busses, or from lower parts of the software stack.

KUKSA clients (feeders & applications) are using the VISS protocol to talk to KUKSA.val. Transport layer security is provided by TLS. JWT tokens are used to enforce access rights on signal level, so that each client only has read and write access to a well defined subset of the VSS model.

## Data Feeders 
Components populating the VSS tree are called  Data feeders. Technically they are just normal KUKSA.val clients. Usually a feeder will gather some data from a vehicle using any kind of standard or propreatry protocol, convert its repreentation the the one mandated by VSS and set the VSS signal.

The following picure shows different options for KUKSA.val feeders 

![Feeder options](./pictures/sysarch_feeders.svg)

We assume running KUKSA.val on a vehicle computer. Some signals might originate in an embedded ECU only connected via CAN (ECU 1). Published data is received by the vehicle computer an can be mapped to the VSS data model. A feeder (DBC Feeder) allowing mapping of data on a CAN bus based on a DBC description and some mapping rules is [included in KUKSA.val](../kuksa_feeders/dbc2val/).

Other ECUs with Ethernet connectivity might publish data as SomeIP services. Following the pattern of the CAN deeder, a SomeIP feeder mapping data to the VSS model can be created.

Some data might originate in the same compute unit running the KUKSA.val server. In this exampe we assume the Vehicle Computer runs an adaptive AutoSAR subsystem. In that case a feeder using the APIs of the underlying system can be created, wihtout the need to parse a specific serialisation first (ara::com Feeder).

Finally, there might be other  processor based platforms in the car such as nother vehicle computer, domain, or zone controller or an infotainment system (VCU). These systems can feed data by interacting wiht the remote KUKSA.val server by using the VISS protocol directly. 

Any feeder can make use of the [KUKSA.val client library](../kuksa_viss_client/) to interact with KUKSA.val using VISS.

## Applications
Applications are accessing vehicle signals through KUKSA.val. The following figure shows common patterns.

![Application patterns](./pictures/sysarch_applications.svg)

A standard application ("Application)"  will request data from KUKSA.val and "do something" with them. An example is gathering some data and visualizing them. The [node.red examples](../kuksa_apps/node-red) in KUKSA.val are an example of this pattern.

Often data needs to be made available in the cloud, or other vehicle-externa system. Especially when connected via cellular networks, behind NATs and using dynamic IP addresses, the VISS patthern of contacting a VSS server is not a good fit. Usually the pattern is, that the vehicle pushes relevant data, using suitbale IoT protocols and potentially dealing with buffering and connection loss. This is the "Cloud Adapter" pattern. An example is the [KUKSA.val S3 uploader](../kuksa_apps/s3/).

As the VSS standard catalogue already contains signals of different abstraction levels, and custom signals can be added, often higher level signals depend on lower level ones. Thus a KUKSA.val application might use data from some signals to calculate the value of of others (Signal Refinement). An example is, taking current battery voltage and other low level signals form the batter system to calcuate a state-of-charge and pushing it back. Another service might use state of-charge, system and ambient temperature to calculate remaining range.


## Where to deploy KUKSA.val in a vehicle

The idea behind VSS and KUKSA.val is, to provide a single entry point to all vehicle data. As in a real vehicle data is distributed and not all domains need access to all data you might opt to run several servers, each providing a subset of data. This can also increase resiliency of the system and allows to seperate safety domains.

Waht is a suitable deployment is very much dependent on the vehicle architecture and the scope of data managed in VSS.

The following figure shows possible patterns
![Deployment patterns](./pictures/sysarch_deployment.svg)

We assume the primary vehicle computer contains the fully populated VSS model for that vehicle. That is probably the simplest deployment and expected in simple vehicle, that might only have one computer able to run KUKSA.val and protoocols such as VISS.

In a redundancy scenario there may be a backup vehicle computer with a KUKSA.val server instance that is kept in sync, so that in case of a failover, the last known data is available for all VSS data points. We assume an optimized VSS Sync component to be used that might deal with one-ay or two-way syncs. Currently KUKSA.val does not include a sync mechanism.

There might be  domain controllers such as the powertrain controller. It might use VSS for internal data not relevnt to other domains and extending it with VSS data provided by the central Vehicle computer.

The Infotainment/Displays example augments this pattern, where the system subscribes needed data fomr the central Vehicle Computer (e.g. data that needs to be visualized), but also feeds data back to main VSS model (e.g. if a driver sets charge limits in the UI). 






