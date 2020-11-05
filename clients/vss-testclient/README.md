# VSS Test Client in Python

## Introduction
This is a command-line test client for the VSS Server developed in Python.


## Requirements
- Python 3.8 
- websockets
- Cmd2

Alternatively, Pipfile/Pipfile.lock can be used if Pipenv is installed.

## Execution 
### With Pipenv
```sh
pipenv sync
pipenv run python testclient.py
```
### Without Pipenv
Ensure all the dependencies are installed.
```sh
pip3 install websockets cmd2 pygments
python3 testclient.py
```

### Usage
Set the Server IP Address & Connect
```
VSS Client> setServerIP 127.0.0.1 8090
VSS Client> connect
```

Refer help for further issues
```
VSS Client> help -v
```

### Docker
You can build a docker image of the testclient. Not the most effcient way to pack a small python script, but it is easy to get started. The Dockerfile needs to be executed on the parent directory (so it include the common library), and it expects a folder "tokens" to be there.
See the  `builddocker.sh` script for an example how to build a docker including the default authorization tokens.

To run it use somehting like this

```
sudo docker run --rm -it --net=host <image-id-from docker-build>
```

`--rm` ensures we do not keep the docker continer lying aroind after closing the vss-testclient and `--net=host` makes sure you can reach locally running kuksa.val-server or kuksa-val docker with port forwarding on the host using the default `127.0.0.1` address.


