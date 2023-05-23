# Running KUKSA.val docker

Once Docker image is built, we can run it by using the  run command:

```bash
docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/kuksa-val:master
```

If you want to use a locally built docker image, make sure to adapt the image name accordingly.

IN this command `$HOME/kuksaval.config` is a directory on your host machine containing the KUKSA.val configuration. The directory can be empty, then it will be populated with an example configuration during start.

The environment variable `KUKSAVAL_OPTARGS` can be used to add arbitrary [command line arguments](usage.md) e.g.

```bash
docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL -e KUKSAVAL_OPTARGS="--insecure" ghcr.io/eclipse/kuksa.val/kuksa-val:master
```
This example enables the insecure mode (i.e. disables TLS). This may be be useful for debugging, if you want to check messages exchanged between val-server and a client in a network analyzer such as Wireshark.