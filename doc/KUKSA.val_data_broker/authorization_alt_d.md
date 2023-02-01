# Authorization

Authorization in kuksa.val is modelled on the OAuth2 specification, or more specifically the
JSON Web Token Profile for OAuth 2.0 Access Tokens.

Using access tokens in this way is well suited for kuksa.val (acting as a resource server),
as it minimizes the need for implementing bespoke code and infrastructure for verifying the
authenticity of clients and their authorization claims.

By aligning with the practices specified in these standards, there's a greater chance
for seamless interoperability with existing authorization solutions.

## Background

Here follows a few snippets from [RFC 6749](https://www.rfc-editor.org/rfc/rfc6749) describing
the OAuth2 framework followed by the more specific RFC 9068 describing the JSON Web Token
Profile for OAuth 2.0 Access Tokens.

### The OAuth 2.0 Authorization Framework [[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1)]
#### Introduction

In the traditional client-server authentication model, the client
requests an access-restricted resource (protected resource) on the
server by authenticating with the server using the resource owner's
credentials.  In order to provide third-party applications access to
restricted resources, the resource owner shares its credentials with
the third party.  This creates several problems and limitations:

* Third-party applications are required to store the resource
  owner's credentials for future use, typically a password in
  clear-text.

* Servers are required to support password authentication, despite
  the security weaknesses inherent in passwords.

* Third-party applications gain overly broad access to the resource
  owner's protected resources, leaving resource owners without any
  ability to restrict duration or access to a limited subset of
  resources.

* Resource owners cannot revoke access to an individual third party
  without revoking access to all third parties, and must do so by
  changing the third party's password.

* Compromise of any third-party application results in compromise of
  the end-user's password and all of the data protected by that
  password.

OAuth addresses these issues by introducing an authorization layer
and separating the role of the client from that of the resource
owner.  In OAuth, the client requests access to resources controlled
by the resource owner and hosted by the resource server, and is
issued a different set of credentials than those of the resource
owner.

Instead of using the resource owner's credentials to access protected
resources, the client obtains an access token -- a string denoting a
specific scope, lifetime, and other access attributes.  Access tokens
are issued to third-party clients by an authorization server with the
approval of the resource owner.  The client uses the access token to
access the protected resources hosted by the resource server.

#### Access Token [[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1.4)]

Access tokens are credentials used to access protected resources.  An
access token is a string representing an authorization issued to the
client.  The string is usually opaque to the client.  Tokens
represent specific scopes and durations of access, granted by the
resource owner, and enforced by the resource server and authorization
server.

The token may denote an identifier used to retrieve the authorization
information or may self-contain the authorization information in a
verifiable manner (i.e., a token string consisting of some data and a
signature).  Additional authentication credentials, which are beyond
the scope of this specification, may be required in order for the
client to use a token.

The access token provides an abstraction layer, replacing different
authorization constructs (e.g., username and password) with a single
token understood by the resource server.  This abstraction enables
issuing access tokens more restrictive than the authorization grant
used to obtain them, as well as removing the resource server's need
to understand a wide range of authentication methods.
   


#### Roles [[RFC 6749](https://www.rfc-editor.org/rfc/rfc6749#section-1.1)]

OAuth defines four roles:

* **resource owner**
  
  An entity capable of granting access to a protected resource.
  When the resource owner is a person, it is referred to as an
  end-user.

* **resource server**
  
  The server hosting the protected resources, capable of accepting
  and responding to protected resource requests using access tokens.

* **client**
  
  An application making protected resource requests on behalf of the
  resource owner and with its authorization.  The term "client" does
  not imply any particular implementation characteristics (e.g.,
  whether the application executes on a server, a desktop, or other
  devices).

* **authorization server**
  
  The server issuing access tokens to the client after successfully
  authenticating the resource owner and obtaining authorization.
      

### JSON Web Token (JWT) Profile for OAuth 2.0 Access Tokens [[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#section-1)]

This specification defines a profile for issuing OAuth 2.0 access tokens in JSON Web Token (JWT) format. Authorization servers and resource servers from different vendors can leverage this profile to issue and consume access tokens in an interoperable manner.

The original OAuth 2.0 Authorization Framework [RFC6749] specification does not mandate any specific format for access tokens. While that remains perfectly appropriate for many important scenarios, in-market use has shown that many commercial OAuth 2.0 implementations elected to issue access tokens using a format that can be parsed and validated by resource servers directly, without further authorization server involvement.

This specification aims to provide a standardized and interoperable profile as an alternative to the proprietary JWT access token layouts going forward. Besides defining a common set of mandatory and optional claims, the profile provides clear indications on how authorization request parameters determine the content of the issued JWT access token, how an authorization server can publish metadata relevant to the JWT access tokens it issues, and how a resource server should validate incoming JWT access tokens.

[...]

Authorization Claims [[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#name-authorization-claims)]

If an authorization request includes a scope parameter, the corresponding issued JWT access token SHOULD include a "scope" claim as defined in Section 4.2 of [RFC8693].
All the individual scope strings in the "scope" claim MUST have meaning for the resources indicated in the "aud" claim. See Section 5 for more considerations about the relationship between scope strings and resources indicated by the "aud" claim.


## Authorization in kuksa.val

Authorization is achieved by supplying an authorization header as part of any http2 / gRPC request.

`Authorization: Bearer TOKEN`

The bearer token is encoded as a JWT access token.

> **JWT access token**:<br>
    An OAuth 2.0 access token encoded in JWT format and complying with the requirements described in this specification. [[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#section-1.2)]

This token contains everything the server needs to verify the authenticity of the claims
contained therein. The structure is specified in the RFC as:

#### JWT Access Token - Data Structure

| Claim     | Description                                          |
|-----------|------------------------------------------------------|
| iss       | REQUIRED - as defined in Section 4.1.1 of [RFC7519]. |
| exp       | REQUIRED - as defined in Section 4.1.4 of [RFC7519]. |
| aud       | REQUIRED - as defined in Section 4.1.3 of [RFC7519]. See Section 3 for indications on how an authorization server should determine the value of "aud" depending on the request. |
| sub       | REQUIRED - as defined in Section 4.1.2 of [RFC7519]. In cases of access tokens obtained through grants where a resource owner is involved, such as the authorization code grant, the value of "sub" SHOULD correspond to the subject identifier of the resource owner. In cases of access tokens obtained through grants where no resource owner is involved, such as the client credentials grant, the value of "sub" SHOULD correspond to an identifier the authorization server uses to indicate the client application. See Section 5 for more details on this scenario. Also, see Section 6 for a discussion about how different choices in assigning "sub" values can impact privacy. |
| client_id | REQUIRED - as defined in Section 4.3 of [RFC8693].   |
| iat       | REQUIRED - as defined in Section 4.1.6 of [RFC7519]. This claim identifies the time at which the JWT access token was issued. |
| jti       | REQUIRED - as defined in Section 4.1.7 of [RFC7519]. |

#### JWT Access Token - Authorization claims
By specifying a scope when requesting an access token (from a authorization server),
the user of the token (client) can be limited in what they're allowed to do.

This is represented in the ["Authorization Claims"](https://datatracker.ietf.org/doc/html/rfc9068#name-authorization-claims) part of the JWT access token.

| Claim     | Description                                          |
|-----------|------------------------------------------------------|
| scope     | If an authorization request includes a scope parameter, the corresponding issued JWT access token SHOULD include a "scope" claim as defined in Section 4.2 of [RFC8693]. |
 |

The value of the scope parameter is expressed as a list of space-delimited, case-sensitive (scope) strings.

> All the individual scope strings in the "scope" claim MUST have meaning for the resources indicated in the "aud" claim. See Section 5 for more considerations about the relationship between scope strings and resources indicated by the "aud" claim. [[RFC 9068](https://datatracker.ietf.org/doc/html/rfc9068#section-2.2.3)]

Example of the claims available in the access token:

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

### Structure of the scope (strings) in kuksa.val

Examples of other implementations:
* https://auth0.com/docs/get-started/apis/scopes/api-scopes
* https://api.slack.com/scopes
* https://docs.github.com/en/developers/apps/building-oauth-apps/scopes-for-oauth-apps

Examples of kuksa.val scope strings:

| Scope string             | Access                                        |
|--------------------------|-----------------------------------------------|
|`read:Vehicle.Speed`      | Client can only read `Vehicle.Speed`          |
|`read:Vehicle.ADAS`       | Client can read everything under Vehicle.ADAS |
|`actuate:Vehicle.ADAS`    | Client can actuate all actuators under Vehicle.ADAS |
|`provide:Vehicle.Width`   | Client can provide `Vehicle.Width`            |
|`!read:Vehicle.Sensitive` | Client is _not_ allowed to read anything under `Vehicle.Sensitive` |

The format of the scope has this general structure.

| Scope                 | Description                 |
|-----------------------|-----------------------------|
|`<ACTION>:<PATH>`      | Allow ACTION for PATH       |
|`!<ACTION>:<PATH>`     | Deny ACTION for PATH        |

with these available actions (initially):

| Action    | Description                                                          |
|-----------|----------------------------------|
| `read`    | Allow reading matching signals   |
| `actuate` | Allow actuating matching signals |
| `provide` | Allow providing matching signals |

To allow the creation of new signals, `create` could be an appropriate action name.

To allow editing the metadata of a signal `edit_metadata` could be an appropriate action name.


### Example 1

Allow reading and actuating all signals below `Vehicle.ADAS`.

```
{
    ...
    "scope": "read:Vehicle.ADAS actuate:Vehicle.ADAS"
}
```

### Example 2

Allow reading all signals under `Vehicle.ADAS` except the subtree of `Vehicle.ADAS.Sensitive`.

```
{
    ...
    "scope": "!read:Vehicle.ADAS.Sensitive read:Vehicle.ADAS"
}
```

### Example 3

Allow reading & providing all entries matching `"Vehicle.Body.Windshield.*.Wiping"`.

```
{
    ...
    "scope": "read:Vehicle.Body.Windshield.*.Wiping provide:Vehicle.Body.Windshield.*.Wiping"
}
```

### Possible extension: using "access_mode" + "fields" for more granularity
The examples above use "actions" to specify what a scope allows/denies. These can be thought of as
a group of permissions needed to perform the intended action. But it's possible to combine this
with an extended syntax that enables replacing "actions" (and "paths") with something else.

Possible future extensions:
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

### Possible extension: Using "tags" instead of "paths"
Another possible extension is to allow something other than "paths" to identify a (group of)
signal(s).

Once scenario is that categorization information (tags) has been added to the VSS .vspec files.
This information would be available as metadata for signals and could then be used to
identify / select signals without the need to know their paths.

This isn't something that is supported today (and might never be), but it's included here to
demonstrate the extensibility of the scope syntax as shown here.

Possible future extensions:
| Scope                                    | Description                                                   |
|------------------------------------------|---------------------------------------------------------------|
|`<ACCESS_MODE>:field:<FIELD>:tag:<PATH>`  | Allow ACCESS_MODE for field FIELD for signals tagged with TAG |
|`!<ACCESS_MODE>:field:<FIELD>:tag:<PATH>` | Deny ACCESS_MODE for field FIELD for signals tagged with TAG  |
|`<ACTION>:tag:<TAG>`                      | Allow ACTION for signals tagged with TAG                      |
|`!<ACTION>:tag:<TAG>`                     | Deny ACTION for signals tagged with TAG                       |

Example:

```
"!read:tag:restricted !actuate:tag:restricted read:tag:low_impact actuate:tag:low_impact"
```


# References

[OAuth Scopes](https://oauth.net/2/scope/)
[JWT Access Tokens](https://oauth.net/2/jwt-access-tokens/)

[API Scopes](https://auth0.com/docs/get-started/apis/scopes/api-scopes)

[Framing the Problem: JWT, JWT ATs Everywhere](https://auth0.com/blog/how-the-jwt-profile-for-oauth-20-access-tokens-became-rfc9068/#Framing-the-Problem--JWT--JWT-ATs-Everywhere)

[The OAuth 2.0 Authorization Framework [RFC 6749]](https://www.rfc-editor.org/rfc/rfc6749)

[JSON Web Token (JWT) Profile for OAuth 2.0 Access Tokens [RFC 9068]](https://datatracker.ietf.org/doc/html/rfc9068)
