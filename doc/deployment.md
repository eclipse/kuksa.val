# Deployment Blueprints


- [Deployment Blueprints](#deployment-blueprints)
- [Deployed Elements](#deployed-elements)
  - [KUKSA.val databroker (or server)](#kuksaval-databroker-or-server)
  - [VSS model](#vss-model)
  - [KUKSA.val Clients](#kuksaval-clients)
- [Deployment Blueprint 1: Internal API](#deployment-blueprint-1-internal-api)
- [Deployment Blueprint 2: Exposing a subset of VSS to another system](#deployment-blueprint-2-exposing-a-subset-of-vss-to-another-system)
- [Deployment Blueprint 3: Individual Applications](#deployment-blueprint-3-individual-applications)
- [Deployment Blueprint 4: Dynamic Applications and VSS extensions](#deployment-blueprint-4-dynamic-applications-and-vss-extensions)
- [Mixing](#mixing)


KUKSA.val offers you an efficient way to provide VSS signals in a vehicle computer. KUKSA.val offers great flexibility how to use and configure it. While we do not know your use case, we want to discuss some "deployment blueprints" based on some use cases that vary in terms of static/dynamic configuration and security setup.

In the examples we will also point out topics that need to be solved outside of KUKSA.val. For example KUKSA.val provides you with the means to use mechanisms such as TLS or cryptographically signed authorization tokens, however solutions and processes for key management of such cryptographic material are out of scope.

# Deployed Elements
On a high level the following (deployment) artifacts are relevant in a KUKSA.val system.

## KUKSA.val databroker (or server)
This is the main component. It provides the API to access and manage VSS datapoints and knows the complete VSS tree. It can be easily deployed as an OCI container, as plain binary, or built from source and deployed as part of your Linux distribution (e.g. Yocto). Since the released distributed binary is statically linked and uses MUSL, the only requirement is that it's built for the right architecture and OS.

## VSS model
KUKSA.val needs to know the VSS datapoints it shall serve. This can be configured upon startup (using a COVESA VSS compliant JSON), or during runtime by adding to / extending the tree.
(Currently databroker only supports adding individual datapoints, server also allows loading complete JSONs as overlay during runtime).

## KUKSA.val Clients
KUKSA.val clients are basically all software components using the KUKSA.val API. Conceptually there are three kinds of KUKSA.val clients.

![Deployment artifacts](./pictures/deployment_artifacts.svg)

See also the [Terminology](./terminology.md) documentation for a deeper discussion

 * **Northbound Consumer**:
 An application interacting with the VSS tree to read signals and potentially intending to actuate actuators.
 * **Southbound Provider**: A component intends to sync the state of the physical vehicle with the VSS model of the server, by providing actual values to VSS sensors or by making sure actuation requests on VSS actuators will be executed.

Technically these clients are all the same, and a single executable may fulfill several or all of these roles at the same time. Conceptually it makes sense to do this differentiation between different intents of interacting with the VSS tree.

Intuitively, it can be seen that the security and safety requirements in an end-2-end system differ for these roles.


# Deployment Blueprint 1: Internal API

You are using VSS and KUKSA.val as an internal API in your system to ease system development. You control the whole system/system integration. 

![Deployment Blueprint 1: Internal API](./pictures/deployment_blueprint1.svg)

| Aspect           | Design Choice |
| ---------------- | ------------- |
| Users            | You           | 
| System Updates   | Complete      | 
| Security         | None/Fixed    | 
| VSS model        | Static        | 
| KUKSA deployment | Firmware      | 

You are not exposing any VSS API to external parties, you control all components interacting with VSS, the system state/composition is under your control. In this case you make KUKSA APIs available only within your system. You might even opt to disabling encryption for higher performance and not using any tokens for authentication. We would still recommend leaving basic security measures intact, but this deployment does not need fine-grained control of permission rights or fast rotation/revocation of tokens.

# Deployment Blueprint 2: Exposing a subset of VSS to another system

You control the system (e.g. Vehicle Computer), that has KUKSA.val deployed. You want to make a subset of capabilities available to another system or middleware, that may have its own API/security mechanisms. An example would be an Android Automotive based IVI system, 

![Deployment Blueprint 2: Exposing a subset of VSS to another system](./pictures/deployment_blueprint2.svg)

| Aspect            | Design Choice |
| ----------------- | ------------- |
| Users             | Other trusted platforms             |
| System Updates    | Firmware on controlled system, unknown on third party system      | 
| Security          | TLS+Foreign platform token    | 
| VSS model         | Static        | 
| KUKSA deployment | Firmware or Software package       | 

In this deployment the foreign system is treated as a single client, which has a certain level of access and trust. Whether that system restricts access further for certain hosted apps is opaque to KUKSA.val. In this deployment you need to enable and configure TLS to provide confidentiality, as well as providing a single token limiting the access of the foreign platform.

If the token is time limited, the foreign platform needs to be provided with a new token in time. In case certificates on the KUKSA.val side are changed, the foreign system needs to be updated accordingly. 

Doing so, you are using KUKSA to enable other ecosystems, while making the usage of KUKSA transparent to application in that ecosystem.

# Deployment Blueprint 3: Individual Applications

You control the system running KUKSA.val. You intend to integrate applications from different vendors/classes of users.  There may be your own "trusted" application as in the *Internal API* blueprint, applications from partners (e.g. pay as you drive insurance) or applications from other third parties, you may not even kow when first deploying a system.

![Deployment Blueprint 3: Individual Applications](./pictures/deployment_blueprint3.svg)

| Aspect           | Design Choice           |
| ---------------- | ------------- |
| Users            | 3rd parties            |     | 
| System Updates   | App-level      | 
| Security         | TLS+Individual Consumer  tokens    | 
| VSS              | Static        | 
| KUKSA deployment | Software package        | 

In this deployment you need to enable and configure TLS to provide confidentiality, as well as providing an individual security token to each VSS consumer, limiting the access of each app.

The scope (and longevity) of those tokens will likely depend on the relationship with the partner providing the component. You probably need some mechanism to revoke tokens (a blacklist is currently not supported in KUKSA.val).

When you update/rotate the keys for the VSS server you need a process to make sure to update customers at the same time.

In this blueprint you expose an attack surface to a larger group of potential adversaries. To be able to react to security issues faster, you might want to deploy KUKSA.val in a way that allows you to update it individually (i.e. deploy it as a container), instead of just baking it into firmware. 


# Deployment Blueprint 4: Dynamic Applications and VSS extensions

This is similar to the *Individual Applications* Use Case with the added capability, that applications might bring their own VSS signals. This may be a connected app that gathers data from the cloud and provides some new weather and road condition datapoints, or maybe an app for controlling a trailer or agricultural implement. To enable this, the application may need to include/require additional providers.


![Deployment Blueprint 4: Dynamic Applications and VSS extensions](./pictures/deployment_blueprint4.svg)

| Aspect         | Design Choice           |
| -------------- | ------------- |
| Users          | 3rd parties           |
| System Updates | App-level      | 
| Security       | Individual App  tokens    | 
| VSS            | Dynamic        | 
| KUKSA deployment | Software package        | 

The main difference to the previous use case is allowing providers to add datapoints to the VSS tree managed by KUKSA.val.

This could be a very elegant setup, even in a more static deployment, where KUKSA.val starts with an empty tree, and once the relevant software components come up, it is extended step by step. 

However, the security and safety implications in such a scenario are the highest: While the security aspect can be handled by KUKSA.val, giving specific applications the right to extend the tree, the overall requirements on system design get harder:

 * Deploying a combination of components, that was not known during development of the vehicle,  that interact with a vehicle raises safety issues.
 * Your SOTA mechanism must make sure, that only "valid" combinations of components are deployed (e.g. the right provider for a specific vehicle enabling a certain consumer, not interfering with another consumers and providers).
 * As neither the full composite VSS tree nor specific software combinations are known a-priori, the system might need the ability to ascertain at runtime whether it is in a safe and usable state.


 # Mixing
 The aforementioned blueprints are examples and any specific deployment might combine aspects of several of them.
 
 There could be many domains in a vehicle, where a fully static "walled" deployment as described in Blueprint 1 is the right thing to do, while other use cases in the same vehicle require  the capabilities found in Blueprint 4. In that case, the right design choice could be to deploy several KUKSA.val instances in a car as sketched in [System Architecture -> Where to deploy KUKSA.val](./system-architecture#where-to-deploy-kuksaval-in-a-vehicle).