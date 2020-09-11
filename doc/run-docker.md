# Running KUKSA.val docker

Once Docker image is built, we can run it by using the  run command:

```bash
sudo docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL amd64/kuksa-val:0.1.1
```
where `$HOME/kuksaval.config` is a directory on your host machine containing the KUKSA.VAL configuration. The directory can be empty, then it will be populated with an example configuration during start.

The environment variable `KUKSAVAL_OPTARGS` can be used to add arbitrary [command line arguments](usage.md) e.g.

```bash
sudo docker run -it --rm -v $HOME/kuksaval.config:/config  -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL -e KUKSAVAL_OPTARGS="--insecure" amd64/kuksa-val:0.1.1
```
