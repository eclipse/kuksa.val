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

Start the replay script, you must always provide a path to a valid log file 

```
$ python3 _replay.py /path/to/logfile.log.csv
```

Use option -g

```
$ python3 _replay.py /path/to/logfile.log.csv -g
```

 to also replay the get Value accesses to the server (might overstress the server)
