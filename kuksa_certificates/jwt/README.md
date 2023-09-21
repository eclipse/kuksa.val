# KUKSA Server JWT Tokens

This directory contains example tokens for communication with the KUKSA Server.
Corresponding tokens for Databroker can be found [here](../../jwt).

The script [createToken.py](createToken.py) can be used to generate new tokens if needed.

For historical reasons KUKSA Server tokens use a different pattern for token names.
If you generate new tokens for KUKSA Server you must first run the script and then rename the generated token.

## Example: Re-generate token with new expiry date

The current example tokens expire at 2025-12-31. To generate new ones you must first add a new expiry date in
the corresponding `*.json files. Then do:

```
./createToken.py single-read.json
mv single-read.token single-read.json.token
```

*Note: The renaming is only needed for KUKSA Server tokens, not for KUKSA Databroker tokens!*
