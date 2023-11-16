# KUKSA Python SDK (kuksa-client)
![kuksa.val Logo](https://raw.githubusercontent.com/eclipse/kuksa.val/0.2.5/doc/pictures/logo.png)


**The KUKSA Python SDK has been moved to a [new repository](https://github.com/eclipse-kuksa/kuksa-python-sdk)!**

## Migration Guide

KUKSA Python SDK 0.4.2 has been released from the repository, and shall be functionally equivalent to 0.4.1
releases from this repository. Minor differences do however exist, like location of certificates and tokens.

### PyPI

If you access KUKSA Python SDK as [PyPI package](https://pypi.org/project/kuksa-client/) no changes are needed.

### Docker container

If you use pre-built docker containers you should still use the old location for versions up to 0.4.1,
but the new location for versions from 0.4.2 onwards.

* [Old location](https://github.com/eclipse/kuksa.val/pkgs/container/kuksa.val%2Fkuksa-client)
* [New location](https://github.com/eclipse-kuksa/kuksa-python-sdk/pkgs/container/kuksa-python-sdk%2Fkuksa-client)

Note that the default branch name has changed, so if you currently use `kuksa-client:master` you must use
`kuksa-client:main` in the new repository
