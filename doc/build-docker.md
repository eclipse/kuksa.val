
# Build Docker image

You can build a docker image for KUKSA.val go to the root of your working copy and execute

```
docker  build  -f ./docker/Dockerfile -t arm64/kuksa-val:myversion  .
```

To build the testclient, it expects some default JWT tokens in the current build context. For details check the vss-client [build.sh script](../clients/vss-testclient/build.sh).

You can cross compile an image for other platforms such as `arm64` using dockers `buildx` feature. To set up buildx check the official documentation https://docs.docker.com/buildx/working-with-buildx/.

If you have a running buildx setup you can create an arm64 image using

```bash
docker buildx build --platform=linux/arm64 -f ./docker/Dockerfile -t arm64/kuksa-val:myversion  .
```


## Development Docker
Not optimized at all, just gets  Ubuntu installed and does a first compile. Will drop you to a shell

To build go to kuksa.val main dir and do

```
docker build -f docker/Dockerfile.dev -t kuksavaltest . 
```


## Advanced docker
_Warning_: This section may be out-of date

Beside all of dependencies already prepared, container image has additional build scripts to help with building of W3C-Server.
Depending on compiler needed, scripts below provide simple initial build configuration to be used and|or extended upon:
 - [docker/build-gcc-default.sh](docker/build-gcc-default.sh),
 - [docker/build-gcc-coverage.sh](docker/build-gcc-coverage.sh),
 - [docker/build-clang-default.sh](docker/build-clang-default.sh),
 - [docker/build-clang-coverage.sh](docker/build-clang-coverage.sh)

In container, scripts are located by default in the `/home/` directory.
Both [build-gcc-default.sh](docker/build-gcc-default.sh) and [build-clang-default.sh](docker/build-clang-default.sh) scripts accept parameters which will be provided to CMake configuration, so user can provide different options to extend default build (e.g add unit test to build).

**Note:** Default path to git repo in the scripts is set to `/home/kuksa.invehicle`. If host path of the git repo, or internally cloned one, is located on different container path, make sure to update scripts accordingly.

