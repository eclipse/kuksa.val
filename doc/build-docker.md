
# Build Docker image

You can build a docker image for KUKSA.val. You can cross compile an image for other platofrms such as `arm64`. To do this, make sure you have installed docker and qemu-user-static

```
sudo apt install qemu-user-static
```

Enable dockers experimental features by creating a `daemon.json`  file in `/etc/docker/` and add

```json
{
  "experimental" : true
}
```
After creeating the configuration you need to restart docker

```
systemctl restart docker
```

To build a docker for an x86/amd64 platform from `kuksa.val` directory do

```
cd docker
sudo  ./build.sh amd64
```

To build an arm64 call the build script like this 

```
sudo  ./build.sh arm64
```

Beware, that this needs a fast computer as compilation runs through qemu (emulating the target processor in software).


In case that network proxy configuration is needed, make sure to export _http_proxy_ and _https_proxy_ environment variables with correct proxy configuration, as shown below:
```
sudo docker build . --build-arg  http_proxy=$http_proxy --build-arg https_proxy=$https_proxy -t w3c-server
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

