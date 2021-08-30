# Usage of kuksa.val replay feature

![kuksa.val Logo](../../doc/pictures/logo.png)

Once you recorded your server in- and outputs to your record file using the [record feature](../../src/VssDatabase_Record.cpp) the replay script can replay the same data with exact timing into the `kuksa.val` server. 

## Usage

### Record a new file

Start `kuksa.val` server providing a record level, for example:

``` bash
$ ./kuksa-val-server --record=recordSetAndGet
```
Provide a different path than the default one using

```
$ ./kuksa-val-server --record=recordGetAndSet --record-path=/path/to/logs
```

### Replay a file to the server

Please configure [config.ini](config.ini) to set log file path and select your Replay mode.

#### Avaliable replay modes are:

|mode|action|
|-|-|
| Set | Replay Set Value only|
| SetGet | Replay Get Value and Set Value to the server |


