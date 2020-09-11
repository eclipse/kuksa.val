# REST API

REST API specification is available as OpenAPI 3.0 definition  in [rest-api.yaml](rest-api.yaml) file.

There is number of options to exercise REST API.

**NOTE:** If using TLS connections and self-signed certificates, make sure that 'CA.pem' or corresponding file to generated certificates is imported into browser (or other tool) used for testing. Also user can try to disable certificate verification.  
Reason for this is that browsers automatically try to verify validity of server certificates, so secured connection shall fail with default configuration._

Similar to above mentioned testclient, there is available [client test page](../test/web-client/index.html) in git repo to aid testing.
Test page support custom GET, PUT and POST HTTP requests to W3C-Server. Additional benefit is that it can automatically generate JWT token based on input token value and provided Client key which is used in authorization. Note that if users changes Client key, user must also update 'jwt.key.pub' with corresponding public key.

Additional tool which is quite useful is [Swagger](https://editor.swagger.io). It is a dual-use tool which allows for writing OpenAPI specifications, but also generates runnable REST API samples for moslient test endpoints.
Open Swagger editor and import our REST API [definition](./rest-api.yaml) file. Swagger shall generate HTML page with API documentation. When one of the endpoints is selected, 'try' button appears which allows for making REST requests directly to running W3C-Server.
