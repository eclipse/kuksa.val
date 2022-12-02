# Deployment Blueprints


* [Deployment Blueprints](#deployment-blueprints)
* [Deployed Elements](#deployed-elements)
   * [KUKSA.val databroker (or server)](#kuksaval-databroker-or-server)
   * [VSS model](#vss-model)
   * [KUKSA.val Clients](#kuksaval-clients)
* [Deployment Blueprint 1: Internal API](#deployment-blueprint-1-internal-api)
* [Deployment Blueprint 2: Exposing a subset of VSS to another system](#deployment-blueprint-2-exposing-a-subset-of-vss-to-another-system)
* [Deployment Blueprint 3: Individual Applications](#deployment-blueprint-3-individual-applications)
* [Deployment Blueprint 4: Dynamic Applications and VSS extensions](#deployment-blueprint-4-dynamic-applications-and-vss-extensions)
* [Mixing](#mixing)


KUKSA.val offers you an efficient way to provide VSS signals in a vehicle computer. It aims to offer great flexibility how to use and configure it. While we do not know your use case, we want to discuss some "deployment blueprints" based on some exemplary use cases that vary in terms of static/dynamic configuration and security setup.

In the examples we will also point out topics that need to be solved outside of KUKSA.val. For example KUKSA.val provides you the means to use mechanisms such as TLS or cryptograhically signed authorisation tokens, however solutions and processes for key management of such cryptographic material are out of scope.

# Deployed Elements
On a high level the following (deployment) artifacts are relevant in a KUKSA.val system

## KUKSA.val databroker (or server)
This is the main component. It provides the API to access and manage VSS datapoints and knows the complete VSS tree. It can be easily deployed as OCI container, as we provide as release but of course you can likewise deploy it as package bake it into your (e.g. Yocto) Linux distribution.

## VSS model
KUKSA.val needs to know the VSS datapoints it shall server. This can be configured upon startup (using a COVESA VSS compliant JSON), or during runtime by adding/extending the tree.
(Currently databroker only supports adding individual datapoints, server also allows loading complete JSON as overlay during runtime)

## KUKSA.val Clients
KUKSA.val clients are basically all software components using the KUKSA.val API. Conceptually there are three kinds of KUKSA.val clients.

![Deployment artifacts](./pictures/deployment_artifacts.svg)

 * **Northbound App**:
 An application interacting with the VSS tree to read and potentially write VSS signals
 * **Southbound Feeder**: A component that is acquiring data from another system (e.g. a CAN bus) transforming it to conform to the desired VSS model and setting it in the tree managed by KUKSA.val
 * **Southbound Service**: A component that subscribes to VSS signals, and depending on their change effects some change in another system (e.g. sending some CAN frames)

Technically these clients are all the same, and a single executable may fulfill several or all of these roles at the same time. Conceptuially it makes sense to do this differentation between different intents of interacting with the VSS tree.

Intuitively, it can be seen that the security and safety loads and requirements in an end-2-end system differ for these roles.


# Deployment Blueprint 1: Internal API

You are using VSS and KUKSA.val as an internal API in your system to ease system development. You control the whole system/system integration. 

![Deployment Blueprint 1: Internal API](./pictures/deployment_blueprint1.svg)

| Aspect         | Design Choice           |
| -------------- | ------------- |
| Users          | You           | 
| System Updates | Complete      | 
| Security       | None/Fixed    | 
| VSS model      | Static        | 
| KUKSA.val deploymment      | Firmware        | 

You are not exposing any VSS API to other players, you control all components interacting with VSS, the system is state/composition is under your control. In this case you would make KUKSA.val available only within your system. You might even forego security such as disabling encryption for higher performance and not using any tokens for authentication. We would still recommend leaving basic security measures intact, but this does not need fine-grained control of permission rights or fast rotation/revocation of tokens.

# Deployment Blueprint 2: Exposing a subset of VSS to another system

You control the system (e.g. Vehicle Computer), that has KUKSA.val deployed. You want to make a subset of capabilites available to another system (e.g. an Android IVI), that may have its own API/security mechanisms. 

From the perspective of the KUKSA.val deployment you already know the consumer at deployment time.

![Deployment Blueprint 2: Exposing a subset of VSS to another system](./pictures/deployment_blueprint2.svg)

| Aspect         | Design Choice           |
| -------------- | ------------- |
| Users          | Other trusted patform           |
| System Updates | N/A      | 
| Security       | Foreign platform token    | 
| VSS            | Static        | 
| KUKSA.val deploymment      | Firmware or Software package       | 

In this deployment the foreign system is treated as a single client, which we has a certain level of access and trust. Whether that system restricts access further for certain hosted apps is opaqe to KUKSA.val. In this deployment you need to enable TLS and configure it accordingly, as well as providing a single token limiting the access of the foreign platform.

If the token is time limited, the foreign platform needs to be provided with a new token in time, in case certificates in the KUKSA.val side are changed, the foreign system needs to be updated accordingly. 

# Deployment Blueprint 3: Individual Applications

You control the system ruinning KUKSA.val. You expose VSS as API but intend to integrate applictions from different vendors/classes of users. There may be you own "trusted" application as in the *Internal API* blueprint, applications from partners (e.g. pay as you drive insurance) or applications from other third parties.

![Deployment Blueprint 2: Individual Applications](./pictures/deployment_blueprint3.svg)

| Aspect         | Design Choice           |
| -------------- | ------------- |
| Users          | 3rd parties           |     | 
| System Updates | App-level      | 
| Security       | Individual App  tokens    | 
| VSS            | Static        | 
| KUKSA.val deployment | Software package        | 


In this case you need to enable TLS and configure it accordingly. You will want to provide security tokens to individual applications. The scope (and longevity) of those tokens will likely depend on the realtionship with the third party provider. You probably need the ability to revoke tokens (a blacklist is currently not supported in KUKSA.val).

When you update/change the keys for KUKSA.val you need a process to make sure to update applications at the same time.

In this blueprint you expose an attack surface to a larger group of potential adversaries. To be able to react on security issues faster, you might want to deploy KUKSA.val in a way that it can be updated individually (i.e. deploy it as a container), instead of just baking it into firmware. 



# Deployment Blueprint 4: Dynamic Applications and VSS extensions

This is similar to the *Indiviudal Applications* Use Case with the added capability, that applications might bring their own datapoints. This may be a connected app that gathers data from the cloud and provides some new weather and road condition datapoints, or maybe an app for controlling a trailer or implement. To work this application may need to include/require additional feeders and VSS services.

| Aspect         | Design Choice           |
| -------------- | ------------- |
| Users          | 3rd parties           |
| Openess        | Available to 3rd parties        | 
| System Updates | App-level      | 
| Security       | Individual App  tokens    | 
| VSS            | Dynamic        | 
| KUKSA.val deployment | Software package        | 

The main difference to the previous use case is the ability allowing applications to add datapoints to the VSS tree managed by KUKSA.val.
This could be a very elegant setup, even in more static deployment, where KUKSA.val starts with an empty tree, and once the relevant software components come up, it is extended step by step. 
However the security and safety implications in such a scenario are the highest: While the security aspect can be handled by KUKSA.val, giving specific applications the right to extend the tree (not yet implemented, on roadmap), the overall requirments on system design get harder

 * Deploying components that interact with a vehicle raises safety issues
 * Your SOTA mechanism must make sure, that only "valid" combinations of components are deployed (e.g. the right feeder for a specific vehicle enabling a certain application, not interfering with another app/feeder/service)
 * As neither (all) the "correct"/valid composite VSS tree and software combinations are known, it is harder for the system to ascertain it is in a safe state


 # Mixing
 The aforementioned blueprints are examples and a specific deplyoment might combine aspects from several of them.
 
 There may be domains in a vehicle, where a fully static "walled" deployment as described in Blueprint 1 is the right thing to do, while there may be use cases in the same car where you want to have the capabilties of Blueprint 4. In that case it might be a valid design choice to deploy several KUKSA.val instances in a car as sketched in [System Architecture -> Where to deploy KUKSA.val](./system-architecture#where-to-deploy-kuksaval-in-a-vehicle).