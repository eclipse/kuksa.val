
# JSON Signing

JSON Signing has been introduced additionally to sign the JSON response for GET and SUBSCRIBE Response. By default this has been disabled. To enable this feature go to visconf.hpp file and uncomment the line `define JSON_SIGNING_ON`. Please note, JSON signing works only with a valid pair of public / private certificate. For testing, you could create example certificates by following the below steps.
Do not add any passphrase when asked for.

```
ssh-keygen -t rsa -b 4096 -m PEM -f signing.private.key
openssl rsa -in signing.private.key  -pubout -outform PEM -out signing.public.key
```

Copy the files signing.private.key & signing.public.key to the build directory.

The client also needs to validate the signed JSON using the public certificate when JSON signing is enabled in server.

This could also be easily extended to support JSON signing for the requests as well with very little effort.
