# spdx

A customized version of [enarx/spdx](https://github.com/enarx/spdx) where files to be checked needs to be explicitly listed.
The intention is to be able to use it for checking files changed in a PR.

## How to use

Please see [check_license.yaml](../workflows/check_license.yaml) for an example on how to use the action to verify
that files modified in a Pull Request has an SPDX license identifier.

It shall be possible to use this action from other repositories by referencing:

`- uses: eclipse/kuksa.val/.github/actions/spdx@master`
