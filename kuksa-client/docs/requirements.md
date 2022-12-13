# Requirements

`kuksa-client` relies on [pip-tools](https://pip-tools.readthedocs.io/en/latest/) to pin requirements versions.
This guide gives you instructions to pin requirements for python3.8 which is the minimum version kuksa-client supports.

## Upgrade requirements

We're using `pip-tools` against our `setup.cfg` file. This means `pip-tools` will make sure that the versions it will pin
match constraints from `setup.cfg`.

First install `pip-tools`:
```console
$ pip install pip-tools
```

Then, check requirements version constraints within `setup.cfg` are still valid or update them accordingly.
Then:

To upgrade requirements versions within `requirements.txt`, do:
```console
$ python3.8 -m piptools compile --upgrade --resolver=backtracking setup.cfg
```

To upgrade requirements versions within `test-requirements.txt`, do:
```console
$ python3.8 -m piptools compile --upgrade --extra=test --output-file=test-requirements.txt --resolver=backtracking setup.cfg
```

If you wish to upgrade individual packages see [Updating requirements](https://pip-tools.readthedocs.io/en/latest/#updating-requirements).
