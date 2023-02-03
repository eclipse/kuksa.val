# Authorization in KUKSA.VAL

* [Background](#background)
* [Introduction to OAuth2](#introduction)
  * [JWT Access Token](#jwt-access-token)
    * [Data Structure](#data-structure)
    * [Authorization claims](#authorization-claims)
* [Examples of other API scope implementations](#examples-of-other-api-scope-implementations)
* [Implementation in KUKSA.VAL](#implementation-in-kuksaval)
  * [Scope format](#scope-format)
    * [Actions](#actions)
    * [Paths](#paths)
    * [Example 1](#example-1)
    * [Example 2](#example-2)
  * [Scope format (possible extensions)](#scope-format-possible-extensions)
    * [Using "deny rules" to limit other broader scopes](#using-deny-rules-to-limit-other-broader-scopes)
    * [Using "access_mode" + "fields" for more granularity](#using-access_mode--fields-for-more-granularity)
    * [Using "tags" instead of "paths"](#using-tags-instead-of-paths)
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

Example of claims available in the access token:

```
{
    "aud": ["kuksa.val.v1", "sdv.databroker.v1"],
    "iss": "https://issuer.example.com",
    "exp": 1443904177,
    "nbf": 1443904077,
    "sub": "dgaf4mvfs75Fci_FL3heQA",
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
principal owner of the resource has certain access rights that needs to be verified by the
resource owner. This can be the user id, group membership, roles or other entitlements,
present in the access token as claims.

In addition to this, the resource owner can specify a scope when requesting an access token
from the authorization server. The result is that the client (e.g. a third party app) is
only granted a subset of the resource owners access rights when using the token.

There are currently no plans in KUKSA.VAL to verify the access rights of the subject itself,
or what kind of access rights a certain group membership or role would provide. What this
means in practice is that anyone authorized to get a valid access token is assumed to have
full access rights within KUKSA.VAL. Access to VSS entries / signals are instead limited
exclusively by the scope provided in the JWT access token.

There is nothing in the design preventing KUKSA.VAL from including support for other types
of access right verifications / mappings in the future. The scope as specified here will
continue to work as a way to limit the access granted by these other means anyway.

### Scope format
The available scope(s) in KUKSA.VAL utilize a concept of "prefix scopes" that consist of
a VSS path, prefixed with an action. This provides a more flexible way to specify scope than
what would be possible using a predefined set of scopes modelled on different use cases.

| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`<ACTION>:<PATH>`      | Allow ACTION for PATH       |

Examples of KUKSA.VAL scope strings:

| Scope string             | Access                                        |
|--------------------------|-----------------------------------------------|
|`read:Vehicle.Speed`      | Client can only read `Vehicle.Speed`          |
|`read:Vehicle.ADAS`       | Client can read everything under Vehicle.ADAS |
|`actuate:Vehicle.ADAS`    | Client can actuate (and read) all actuators under Vehicle.ADAS |
|`provide:Vehicle.Width`   | Client can provide (and read) `Vehicle.Width` |

#### Actions
Available actions (initially):

| Action    | Description                                                          |
|-----------|----------------------------------|
| `read`    | Allow reading matching signals   |
| `actuate` | Allow actuating (and read) matching signals |
| `provide` | Allow providing (and read) matching signals |

To allow the creation of new signals, `create` could be an appropriate action name.

To allow editing the metadata of a signal `edit_metadata` could be an appropriate action name.

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

### Scope format (possible extensions)
#### Using "deny rules" to limit other broader scopes
| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`!<ACTION>:<PATH>`     | Deny ACTION for PATH        |

All "deny" scopes have priority over any "allow" scope.

Examples of KUKSA.VAL scope strings:

| Scope string             | Access                                        |
|--------------------------|-----------------------------------------------|
|`read:Vehicle`            | Client can read everything below `Vehicle` (except `Vehicle.Sensitive` b/c of the scope below)    |
|`!read:Vehicle.Sensitive` | Client is _not_ allowed to read anything under `Vehicle.Sensitive` |

#### Using "access_mode" + "fields" for more granularity
The examples above use "actions" to specify what a scope allows/denies. These can be thought of as
a group of permissions needed to perform the intended action. But it's possible to combine this
with an extended syntax that enables replacing "actions" with something else.

| Scope                                | Description                                |
|--------------------------------------|--------------------------------------------|
|`<ACCESS_MODE>:field:<FIELD>:<PATH>`  | Allow ACCESS_MODE for field FIELD for PATH |
|`!<ACCESS_MODE>:field:<FIELD>:<PATH>` | Deny ACCESS_MODE for field FIELD for PATH  |

Specifying scopes in this way is more verbose, but allows finer grained control. The point is that
these scopes can coexist with the "normal" scopes. By taking advantage of combining "allow" and "deny"
rules, the verbosity can be minimised.

Example:
```
"read:field:value:Vehicle readwrite:field:actuator_target:Vehicle.ADAS"
```

#### Using "tags" instead of "paths"
Another possible extension is to allow something other than "paths" to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS `.vspec` files.
This information would be available as metadata for VSS entries and could be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the scope syntax as shown here.

| Scope                                   | Description                              |
|-----------------------------------------|------------------------------------------|
|`<ACCESS_MODE>:field:<FIELD>:tag:<TAG>`  | Allow ACCESS_MODE for field FIELD for signals tagged with TAG |
|`!<ACCESS_MODE>:field:<FIELD>:tag:<TAG>` | Deny ACCESS_MODE for field FIELD for signals tagged with TAG  |
|`<ACTION>:tag:<TAG>`                     | Allow ACTION for signals tagged with TAG |
|`!<ACTION>:tag:<TAG>`                    | Deny ACTION for signals tagged with TAG  |

Example:

```
"!read:tag:restricted !actuate:tag:restricted read:tag:low_impact actuate:tag:low_impact"
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
