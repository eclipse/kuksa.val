
# KUKSA.VAL usage
Using `./kuksa-val-server -h`, you can get a list of supported options:

```
Usage: ./src/kuksa-val-server OPTIONS
Options:
  -h [ --help ]                         Help screen
  -c [ --config-file ] arg (="config.ini")
                                        Configuration file with W3C-Server 
                                        input parameters.Configuration file can
                                        replace command-line parameters and 
                                        through different files multiple 
                                        configurations can be handled more 
                                        easily (e.g. test and production 
                                        setup).Sample of configuration file 
                                        parameters looks like:
                                        vss=vss_rel_2.0.json
                                        cert-path=. 
                                        insecure=true 
                                        log-level=ALL
                                        
  --vss arg                             [mandatory] Path to VSS data file 
                                        describing VSS data tree structure 
                                        which W3C-Server shall handle. Sample 
                                        'vss_rel_2.0.json' file can be found 
                                        under [unit-test](./unit-test/vss_rel_2
                                        .0.json)
  --cert-path arg (=".")                [mandatory] Directory path where 
                                        'Server.pem', 'Server.key' and 
                                        'jwt.key.pub' are located. 
  --insecure                            By default, W3C-Server shall accept 
                                        only SSL (TLS) secured connections. If 
                                        provided, W3C-Server shall also accept 
                                        plain un-secured connections for 
                                        Web-Socket and REST API connections, 
                                        and also shall not fail connections due
                                        to self-signed certificates.
  --use-keycloak                        Use KeyCloak for permission management
  --address arg (=127.0.0.1)            If provided, W3C-Server shall use 
                                        different server address than default 
                                        _'localhost'_
  --port arg (=8090)                    If provided, W3C-Server shall use 
                                        different server port than default 
                                        '8090' value
  --log-level arg                       Enable selected log level value. To 
                                        allow for different log level 
                                        combinations, parameter can be provided
                                        multiple times with different log level
                                        values.
                                        Supported log levels: NONE, VERBOSE, 
                                        INFO, WARNING, ERROR, ALL

MQTT Options:
  --mqtt.insecure                       Do not check that the server 
                                        certificate hostname matches the remote
                                        hostname. Do not use this option in a 
                                        production environment
  --mqtt.username arg                   Provide a mqtt username
  --mqtt.password arg                   Provide a mqtt password
  --mqtt.address arg (=localhost)       Address of MQTT broker
  --mqtt.port arg (=1883)               Port of MQTT broker
  --mqtt.qos arg (=0)                   Quality of service level to use for all
                                        messages. Defaults to 0
  --mqtt.keepalive arg (=60)            Keep alive in seconds for this mqtt 
                                        client. Defaults to 60
  --mqtt.retry arg (=3)                 Times of retry via connections. 
                                        Defaults to 3
  --mqtt.topic-prefix arg               Prefix to add for each mqtt topics
  --mqtt.publish arg                    List of vss data path (using readable 
                                        format with `.`) to be published to 
                                        mqtt broker, using ";" to seperate 
                                        multiple path and "*" as wildcard

```


Server demo certificates are located in [../examples/demo-certificates](../examples/demo-certificates) directory of git repo. Certificates from 'demo-certificates' are automatically copied to build directory, so invoking '_--cert-path=._' should be enough when demo certificates are used.  
If user needs to use or generate their own certificates, see chapter [_Certificates_](#Certificates) for more details.  
For authorizing client, file 'jwt.key.pub' contains public key used to verify that JWT authorization token is valid. To generated different 'jwt.key.pub' file, see chapter [_Permissions_](#Permissions) for more details.

Default configuration shall provide both Web-Socket and REST API connectivity.
