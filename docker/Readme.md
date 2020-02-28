#  Dockerfiles


Dockerfiles.test ist for setting up an environment to play/develop VSS

## Development Docker
Not optimized a t all, just gets  Ubuntu installed and does a first compile. Will drop you to a shell

To build go to kuksa.val main dir and do

```
docker build -f docker/Dockerfile.dev -t kuksavaltest . 
```

To run

```
docker run -it --net=host kuksavaltest:latest 
```

## Production docker
To run kuksa.val on target hardware use an optimized docker build based on alpine. It is built the "Kuksa-way" allowing easy builds for different architectures.

### Build prerequisites

1. Enable docker experimental feature.

Create a daemon.json file in /etc/docker/ and add 

```
{
  "experimental" : true
}
```

and restart docker daemon using  `sudo systemctl restart docker`

https://github.com/docker/docker-ce/blob/master/components/cli/experimental/README.md

2. install  qemu-user-static package on the host machine

 use 
 
 ```sudo apt-get install qemu-user-static```

### Build

The `Dockerfile` is supposed to 
be run through the magic build.sh wrapper.

To build for amd64 do

`./build.sh amd64`

To build for ARM64 (armv8) do

`/build.sh arm64`

This image can also run on a Raspberry _if_ you installed a 64 bit operating system.

For vanilla Raspbian try (untested) the `arm32v6` parameter

Please note that building for ARM is MUCH slower on an x64 amchien as it is uses qemu emulation. THe created container does not use qemu and therefore can only be run on the correct target hardware


### Run build docker
To start it run e.g.


```
docker run -it  -p 127.0.0.1:8090:8090 --mount type=bind,source=/path/to/myconfig,target=/config amd64/kuksa-val:0.1.1
```

 `/path/to/myconfig ` is a folder where a `vss.json` datamodel is expected and a subdirectory `/path/to/myconfig/certs` containing at least the server key (`Server.key`) and certificate (`Server.pem`). If those files do not exist default files will be copied automatically


