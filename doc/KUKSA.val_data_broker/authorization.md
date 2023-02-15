# Authorization in KUKSA.VAL

* [Background](#background)
* [Introduction to OAuth2](#introduction)
  * [JWT Access Token](#jwt-access-token)
    * [Data Structure](#data-structure)
    * [Authorization claims](#authorization-claims)
* [Examples of other API scope implementations](#examples-of-other-api-scope-implementations)
* [Implementation in KUKSA.VAL](#implementation-in-kuksaval)
  * [Limit access to resources](#limit-access-to-resources)
  * [Limit access with scope](#limit-access-with-scope)
    * [Hierarchical Access Rights](#hierarchical-access-rights)
    * [Actions](#actions)
    * [Paths](#paths)
    * [Example 1](#example-1)
    * [Example 2](#example-2)
  * [Limit access (additional possibilities)](#limit-access-additional-possibilities)
    * [Add "subactions" to scope for more granularity](#add-subactions-to-scope-for-more-granularity)
    * [Add "deny" scopes to limit other scopes](#add-deny-scopes-to-limit-other-scopes)
    * [Add "tag" to scope as alternative to path](#add-tag-to-scope-as-alternative-to-path)
    * [Add "vin" to scope to limit access per vehicle](#add-vin-to-scope-to-limit-access-per-vehicle)
* [References](#references)
  * [The OAuth 2.0 Authorization Framework](#the-oauth-20-authorization-framework)
    * [Introduction](#introduction-1)
    * [Access Token](#access-token)
    * [Roles](#roles)
  * [JSON Web Token (JWT) Profile for OAuth 2.0 Access Tokens]()
    * [Introduction](#introduction-2)
    * [Authorization Claims](#authorization-claims-1)
  * [Links](#links)

## Background
One design goal for the authorization in KUKSA.VAL is that it should be as independent as possible from the authorization infrastructure used around it. As a result, the preferred solution has been to use a signed JWT
access token where everything needed for authorization is contained within. In kuksa-val-server, this information
was contained in the claim `"kuksa-vss"` along with the separate `"admin"` and `"modifyTree"` claims.

```yaml
{
  ...
  "admin": true,
  "modifyTree": false,
  "kuksa-vss":  {
     "Vehicle.Drivetrain.Transmission.DriveType": "rw",
     "Vehicle.OBD.*": "r"
  }
}
 ```

One shortcoming in this design, was the inability to differentiate between the right to actuate signals (i.e. a form of write access) and the right to change the value of the signals (another form of write access).

Another downside was that these authorization claims where not readily compatible with the (now) widely used [OAuth 2.0 Authorization Framework](#the-oauth-20-authorization-framework) and [JWT Profile for OAuth 2.0 Access Tokens](#json-web-token-jwt-profile-for-oauth-20-access-tokens). Using these, the same functionality is instead provided by adding a scope to the request for access tokens from the authorization server which is then included in the issued JWT access token as the `"scope"` claim. Being compatible with these standards would make KUKSA.VAL compatible with a whole range of external authorization providers and libraries.

This document aims to describe a new design for authorization in KUKSA.VAL which would address these shortcomings.
In addition, an attempt is made to provide an orientation for how KUKSA.VAL would fit into the overall OAuth 2.0 Authorization Framework.

# Introduction to OAuth2
Using an authorization framework like OAuth2 is well suited for an environment where third party applications need delegated access to a resource, while at the same time restricting the scope of this access to minimum. 

See [The OAuth 2.0 Authorization Framework](#the-oauth-20-authorization-framework) for a more detailed description of the benefits, and the problems the framework aims to solve.

OAuth 2.0 defines four roles:

* **Resource owner**
  >   An entity capable of granting access to a protected resource.
  >   When the resource owner is a person, it is referred to as an
  >   end-user.
  
  The resource owner in this context could be the OEM or the owner of a vehicle.

* **Resource server**
  > The server hosting the protected resources, capable of accepting
  > and responding to protected resource requests using access tokens.

  The resource server in this context is KUKSA.VAL.

* **Client**
  > An application making protected resource requests on behalf of the
  > resource owner and with its authorization.  The term "client" does
  > not imply any particular implementation characteristics (e.g.,
  > whether the application executes on a server, a desktop, or other
  > devices).

  The client in this context can be apps distributed by the OEM or third parties.

* **Authorization server**
  > The server issuing access tokens to the client after successfully
  > authenticating the resource owner and obtaining authorization.

  The authorization server is most likely an server controlled by
  the OEM and run by either the OEM or a third party (i.e. Bosch).

The resource owner can specify the scope of access rights granted to a client by
including a scope in the request for an access token from the authorization server.
This access token will be given to the client and is sufficient for KUKSA.VAL to
independently determine if the client is allowed access to a certain resource.

*Note:*
*Strictly speaking, KUKSA.VAL also needs to verify that the subject (found in `"sub"`)*
*has the required access rights. See the RFC for more information.*
*The working assumption, for now, is that anyone authorized to get a valid access token*
*has the required access rights*

In addition to the OAuth2 specification, The [JWT Profile for OAuth 2.0 Access Tokens](#json-web-token-jwt-profile-for-oauth-20-access-tokens)
specifies a standardized structure for JWT access tokens. By following this standard,
there's a greater chance for seamless integration with as many existing authorization
solutions as possible.

Finally, by using access tokens for authorization, the amount of responsibility placed
on KUKSA.VAL, acting as a resource server, is minimized in the overall authorization
infrastructure. This should minimize the amount of bespoke code necessary to support a
wide range of authorization scenarios.

## JWT Access Token

JWT access tokens are supplied by adding an authorization header to the HTTP / gRPC requests.

`Authorization: Bearer TOKEN`

The TOKEN is a JWT access token as specified in the [RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068).

> **JWT access token**:<br>
    An OAuth 2.0 access token encoded in JWT format and complying with the requirements described in this specification. [[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#section-1.2)]

The access token contains everything the server needs to verify the authenticity of the claims
contained therein.

Example of what's available in a JWT access token:

Header:

```yaml
{
  "typ": "at+JWT",
  "alg": "RS256",
  "kid": "RjEwOwOA"
}
```

Claims:
```yaml
{
    "aud": [
      "vehicle/5GZCZ43D13S812715/kuksa.val.v1",
      "vehicle/5GZCZ43D13S812715/sdv.databroker.v1"
    ],
    "iss": "https://issuer.example.com",
    "exp": 1443904177,
    "nbf": 1443904077,
    "sub": "dgaf4mvfs7",
    "client_id": "s6BhdRkqt3",
    "scope": "read:Vehicle actuate:Vehicle"
}
```

### Data Structure

| Claim     | Description                                          |
|-----------|------------------------------------------------------|
| iss       | REQUIRED - as defined in Section 4.1.1 of [RFC7519]. |
| exp       | REQUIRED - as defined in Section 4.1.4 of [RFC7519]. |
| aud       | REQUIRED - as defined in Section 4.1.3 of [RFC7519]. See Section 3 for indications on how an authorization server should determine the value of "aud" depending on the request. |
| sub       | REQUIRED - as defined in Section 4.1.2 of [RFC7519]. In cases of access tokens obtained through grants where a resource owner is involved, such as the authorization code grant, the value of "sub" SHOULD correspond to the subject identifier of the resource owner. In cases of access tokens obtained through grants where no resource owner is involved, such as the client credentials grant, the value of "sub" SHOULD correspond to an identifier the authorization server uses to indicate the client application. See Section 5 for more details on this scenario. Also, see Section 6 for a discussion about how different choices in assigning "sub" values can impact privacy. |
| client_id | REQUIRED - as defined in Section 4.3 of [RFC8693].   |
| iat       | REQUIRED - as defined in Section 4.1.6 of [RFC7519]. This claim identifies the time at which the JWT access token was issued. |
| jti       | REQUIRED - as defined in Section 4.1.7 of [RFC7519]. |

### Authorization claims
> If an authorization request includes a scope parameter, the corresponding issued
> JWT access token SHOULD include a "scope" claim as defined in Section 4.2 of [RFC8693].

> All the individual scope strings in the "scope" claim MUST have meaning for the
> resources indicated in the "aud" claim. See Section 5 for more considerations about
>  the relationship between scope strings and resources indicated by the "aud" claim.

| Claim     | Description                                          |
|-----------|------------------------------------------------------|
| scope     | The value of the scope claim is a JSON string containing a space-separated list of scopes associated with the token, in the format described in Section 3.3 of [RFC6749] |

## Examples of other API scope implementations
* https://auth0.com/docs/get-started/apis/scopes/api-scopes
* https://api.slack.com/scopes
* https://docs.github.com/en/developers/apps/building-oauth-apps/scopes-for-oauth-apps

## Implementation in KUKSA.VAL
A common (perhaps the most common) scenario when delegating access using OAuth2 is that the
principal owner of a resource has certain access rights that needs to be verified by the
resource server. This can be accomplished using the user id, group membership, roles or other entitlements present in the access token as claims.

In addition to this, the resource owner can further limit what's authorized by an access
token by specifying a scope in the request to the authorization server. The issued access
token will then only grant this subset of authorization to the client (e.g. a third party app)
using the token.

There are currently no plans in KUKSA.VAL to separately verify the access rights of the
resource owner itself, or what kind of access rights a certain group membership or role would provide. What this means in practice is that anyone authorized to get a valid access token is assumed to have full access rights within KUKSA.VAL, which will only be limited by the scope provided.

There is nothing in the design preventing KUKSA.VAL from including support for other types
of verifications in the future. The scope as specified here will continue to work as a way to limit the access granted by these other means anyway.


### Limit access to resources
The issued access token will typically be intended to for use with a certain resource. In the
context of KUKSA.VAL, this can be a certain vehicle, or a certain backend service.

To prevent access tokens issued for one resource from being used to gain access to other,
unintended resources, OAuth2 introduces the concept of [Resource Indicators [RFC 8707]](https://datatracker.ietf.org/doc/html/rfc8707). By specifying the intended resource in the request
to the authorization server, the resulting `"aud"` or audience claim should be populated with
this resource.

From [RFC9068](https://datatracker.ietf.org/doc/html/rfc9068#JWTATLRequest):
> If the request includes a "resource" parameter (as defined in [[RFC8707](https://datatracker.ietf.org/doc/html/rfc6749)]), the resulting JWT access token "aud" claim SHOULD have the same value as the "resource" parameter in the request.

and from [RFC8707](https://datatracker.ietf.org/doc/html/rfc6749):
> To prevent cross-JWT confusion, authorization servers MUST use a distinct identifier as an "aud" claim value to uniquely identify access tokens issued by the same issuer for distinct resources. For more details on cross-JWT confusion, please refer to Section 2.8 of [RFC8725].

By using a resource identifier that uniquely identifies the resource (i.e. a certain vehicle),
and including this in the `"aud"` claim, KUKSA.VAL can verify that the access token isn't meant
for another resource.

KUKSA.VAL also needs to verify that the access token is intended to be used for KUKSA.VAL.
An `"aud"` claim that would address both these requirements could look something like this (TBD):

```yaml
{
  "aud": [
    "vehicle/5GZCZ43D13S812715/kuksa.val.v1"
  ],
  ...
}
```

where `5GZCZ43D13S812715` could be the VIN or another unique identifier available to KUKSA.VAL
for local verification.

### Limit access with scope
Scopes are intended to limit what is authorized by an access token. To avoid coming up with
scopes of different names for every imaginable use case, KUKSA.VAL uses "prefix scopes" to
allow the dynamic creation of scopes that corresponds to the intent of the authorization.
#### Hierarchical Access Rights
The core concept is that a scope consist of an _action_ that represent the intent. This can be
seen as the top level of an access right hierarchy that allows everything below it. To limit
the scope further, _sub actions_ and _sub selections_ can be added to the right of this prefix.
The different levels are separated by `:`. This provides a flexible way to limit a the scope
for the intended use case.

So for example, `read` is the top level action in an access rights hierarchy and includes read
access to everything. `read:Vehicle.Speed` is an example of a _sub selection_ (a path) that
limits the access rights otherwise granted by the overall `read`. Furthermore, this can be
expanded in the future to include other _sub actions_ such as `read:field:FIELD` which can also
be combined with further _sub selections_ such as the path above or things like tags etc. See
[Limit access (additional possibilities)](#limit-access-additional-possibilities).

| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`<ACTION>[:SUB_ACTION][:<PATH>]`    | Allow ACTION for PATH       |

Examples of KUKSA.VAL scope strings:

| Scope string             | Access                                        |
|--------------------------|-----------------------------------------------|
|`read`                    | Allow client to read everything               |
|`read:Vehicle.Speed`      | Allow client to read `Vehicle.Speed` (including metadata) |
|`read:Vehicle.ADAS`       | Allow client to read everything under Vehicle.ADAS (including metadata) |
|`actuate:Vehicle.ADAS`    | Allow client to actuate all actuators under Vehicle.ADAS (including `read`) |
|`provide:Vehicle.Width`   | Allow client to provide `Vehicle.Width` (including `read`) |

#### Actions
Available actions (initially):

| Action    | Description                                    |
|-----------|------------------------------------------------|
| `read`    | Allow client to read matching signals (and metadata) |
| `actuate` | Allow client to actuate matching signals (includes `read`) |
| `provide` | Allow client to provide matching signals (includes `read`) |
| `create`  | Allow client to create a VSS entry (under a certain path). If a VSS entry already exists, a separate scope (not fully defined yet) is needed to change it. |

| Subactions | Description                     |
|--------------------------|---------------------------------|
| `provide:data` | Allow client to provide data (but not respond to actuation requests) for matching signals. (includes `read`) |
| `provide:actuation` | Allow client to respond to actuation requests (but not provide the value) for matching signals. (includes `read`) |


#### Paths
If the `<PATH>` is a VSS branch, the scope applies to all children of that branch.

If the `<PATH>` contains a wildcard (`*`), the wildcard only matches a single level, i.e. `"Vehicle.*.IsOpen"` would _not_ match `Vehicle.Body.Trunk.Rear.IsOpen`, while `"Vehicle.*.*.*.IsOpen"` however, would.

#### Example 1

Allow reading and actuating all signals below `Vehicle.ADAS`.

```
{
    ...
    "scope": "read:Vehicle.ADAS actuate:Vehicle.ADAS"
}
```

#### Example 2

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping"`.

```
{
    ...
    "scope": "read:Vehicle.Body.Windshield.*.Wiping provide:Vehicle.Body.Windshield.*.Wiping"
}
```

### Limit access (additional possibilities)

#### Add "modify" to allow changing metadata of entries
Suggested alternative naming: `edit`, `update`

| Action    | Description                                     |
|-----------|-------------------------------------------------|
| `modify`  | Allow client to modify metadata of a VSS entry. |

The reason for differentiating between creating and modifying entries is that creating an entry shouldn't have a huge effect on already running applications in the vehicle. Changing an entry on the other hand, could have a huge effect as it has the potential to invalidate the state of a lot of applications. That's why the right to create an entry doesn't automatically include the authorization to edit it.

Note:
Further considerations: Runtime changes vs persistent changes.

#### Add "field" to scope for more granularity

| Subactions for `read` | Description                        |
|-----------------------|------------------------------------|
| `read:field:<FIELD>`  | Allow client to only read a certain field for matching signals |

| Subactions for `edit` | Description                     |
|-----------------------|---------------------------------|
| `edit:field:FIELD` | Allow client to edit metadata field FIELD for matching signals. (includes `read`) |


#### Add "deny" scopes to limit other scopes
| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`!<ACTION>:<PATH>`     | Deny ACTION for PATH        |

All "deny" scopes have priority over any "allow" scope.

Examples of KUKSA.VAL scope strings:

| Scope string             | Access                                        |
|--------------------------|-----------------------------------------------|
|`read:Vehicle`            | Client can read everything under path `Vehicle` (except what is denied below) |
|`!read:field:sensitive_field` | Client is _not_ allowed to read `sensitive_field` |
|`!read:Vehicle.Sensitive.Path` | Client is _not_ allowed to read anything under `Vehicle.Sensitive.Path` |


#### Add "tag" to scope as alternative to path
Another possible extension is to allow something other than paths to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS `.vspec` files.
This information would be available as metadata for VSS entries and could be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the scope syntax as shown here.

| Scope                                   | Description                              |
|-----------------------------------------|------------------------------------------|
|`<ACTION>:field:<FIELD>:tag:<TAG>`  | Allow ACTION for field FIELD for signals tagged with TAG |
|`!<ACTION>:field:<FIELD>:tag:<TAG>` | Deny ACTION for field FIELD for signals tagged with TAG  |
|`<ACTION>:tag:<TAG>`                     | Allow ACTION for signals tagged with TAG |
|`!<ACTION>:tag:<TAG>`                    | Deny ACTION for signals tagged with TAG  |

Example:

```
"!read:tag:restricted !actuate:tag:restricted read:tag:low_impact actuate:tag:low_impact"
```

#### Add "vin" to scope to limit access per vehicle
As described in [Limit access to resources](#limit-access-to-resources), it's a good idea
to limit what resources an access token is authorized to access. The `"aud"` claim seems
to be a good way to do this.

A possible addition to this would be to be able to limit access to certain VIN:s by also
introducing a `"vin:<VIN>"` scope.

```yaml
{
  ...
  "scope": "... vin:5GZCZ43D13S812715"
}
```

# References

## The OAuth 2.0 Authorization Framework

### Introduction

> In the traditional client-server authentication model, the client
> requests an access-restricted resource (protected resource) on the
> server by authenticating with the server using the resource owner's
> credentials.  In order to provide third-party applications access to
> restricted resources, the resource owner shares its credentials with
> the third party.  This creates several problems and limitations:
>
> * Third-party applications are required to store the resource
>   owner's credentials for future use, typically a password in
>   clear-text.
> 
> * Servers are required to support password authentication, despite
>   the security weaknesses inherent in passwords.
> 
> * Third-party applications gain overly broad access to the resource
>   owner's protected resources, leaving resource owners without any
>   ability to restrict duration or access to a limited subset of
>   resources.
> 
> * Resource owners cannot revoke access to an individual third party
>   without revoking access to all third parties, and must do so by
>   changing the third party's password.
> 
> * Compromise of any third-party application results in compromise of
>   the end-user's password and all of the data protected by that
>   password.
>
> OAuth addresses these issues by introducing an authorization layer
> and separating the role of the client from that of the resource
> owner.  In OAuth, the client requests access to resources controlled
> by the resource owner and hosted by the resource server, and is
> issued a different set of credentials than those of the resource
> owner.
>
> Instead of using the resource owner's credentials to access protected
> resources, the client obtains an access token -- a string denoting a
> specific scope, lifetime, and other access attributes.  Access tokens
> are issued to third-party clients by an authorization server with the
> approval of the resource owner.  The client uses the access token to
> access the protected resources hosted by the resource server.

[[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1)]

#### Access Token

> Access tokens are credentials used to access protected resources.  An
> access token is a string representing an authorization issued to the
> client.  The string is usually opaque to the client.  Tokens
> represent specific scopes and durations of access, granted by the
> resource owner, and enforced by the resource server and authorization
> server.
> 
> The token may denote an identifier used to retrieve the authorization
> information or may self-contain the authorization information in a
> verifiable manner (i.e., a token string consisting of some data and a
> signature).  Additional authentication credentials, which are beyond
> the scope of this specification, may be required in order for the
> client to use a token.
> 
> The access token provides an abstraction layer, replacing different
> authorization constructs (e.g., username and password) with a single
> token understood by the resource server.  This abstraction enables
> issuing access tokens more restrictive than the authorization grant
> used to obtain them, as well as removing the resource server's need
> to understand a wide range of authentication methods.
   
[[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1.4)]

#### Roles
> OAuth defines four roles:
> 
> * **resource owner**
>   
>   An entity capable of granting access to a protected resource.
>   When the resource owner is a person, it is referred to as an
>   end-user.
> 
> * **resource server**
>   
>   The server hosting the protected resources, capable of accepting
>   and responding to protected resource requests using access tokens.
> 
> * **client**
>   
>   An application making protected resource requests on behalf of the
>   resource owner and with its authorization.  The term "client" does
>   not imply any particular implementation characteristics (e.g.,
>   whether the application executes on a server, a desktop, or other
>   devices).
> 
> * **authorization server**
>   
>   The server issuing access tokens to the client after successfully
>   authenticating the resource owner and obtaining authorization.

[[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1.1)]

## JSON Web Token (JWT) Profile for OAuth 2.0 Access Tokens

### Introduction

> This specification defines a profile for issuing OAuth 2.0 access tokens in JSON Web Token (JWT) format. Authorization servers and resource servers from different vendors can leverage this profile to issue and consume access tokens in an interoperable manner.
> 
> The original OAuth 2.0 Authorization Framework [RFC6749] specification does not mandate any specific format for access tokens. While that remains perfectly appropriate for many important scenarios, in-market use has shown that many commercial OAuth 2.0 implementations elected to issue access tokens using a format that can be parsed and validated by resource servers directly, without further authorization server involvement.
> 
> This specification aims to provide a standardized and interoperable profile as an alternative to the proprietary JWT access token layouts going forward. Besides defining a common set of mandatory and optional claims, the profile provides clear indications on how authorization request parameters determine the content of the issued JWT access token, how an authorization server can publish metadata relevant to the JWT access tokens it issues, and how a resource server should validate incoming JWT access tokens.

[[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#section-1)]


### Authorization Claims

> If an authorization request includes a scope parameter, the corresponding issued JWT access token SHOULD include a "scope" claim as defined in Section 4.2 of [RFC8693].
> All the individual scope strings in the "scope" claim MUST have meaning for the resources indicated in the "aud" claim. See Section 5 for more considerations about the relationship between scope strings and resources indicated by the "aud" claim.

[[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#name-authorization-claims)]


## Links
1. [OAuth Scopes](https://oauth.net/2/scope/)
2. [JWT Access Tokens](https://oauth.net/2/jwt-access-tokens/)
3. [API Scopes](https://auth0.com/docs/get-started/apis/scopes/api-scopes)
4. [Framing the Problem: JWT, JWT ATs Everywhere](https://auth0.com/blog/how-the-jwt-profile-for-oauth-20-access-tokens-became-rfc9068/#Framing-the-Problem--JWT--JWT-ATs-Everywhere)
5. [The OAuth 2.0 Authorization Framework [RFC 6749]](https://www.rfc-editor.org/rfc/rfc6749)
6. [JSON Web Token (JWT) Profile for OAuth 2.0 Access Tokens [RFC 9068]](https://datatracker.ietf.org/doc/html/rfc9068)
