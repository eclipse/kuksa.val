
# Build Docker image

You can build a docker image for KUKSA.val server go to the root of your working copy and execute

```
docker  build  -f ./kuksa-val-server/docker/Dockerfile -t arm64/kuksa-val:myversion  .
```

You can cross compile an image for other platforms such as `arm64` using dockers `buildx` feature. To set up buildx check the official documentation https://docs.docker.com/buildx/working-with-buildx/.

If you have a running buildx setup you can create an arm64 image using

```bash
docker buildx build --platform=linux/arm64 -f ./kuksa-val-server/docker/Dockerfile -t arm64/kuksa-val:myversion  .
```


## Development Docker
Not optimized at all, just gets  Ubuntu installed and does compile KUKSA Server.
When run it will start KUKSA Server with default configuration.

To build go to `kuksa.val` directory and do

```
docker build -f ./kuksa-val-server/docker/Dockerfile.dev -t kuksavaltest .
```
