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
pip3 install websockets cmd2
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
