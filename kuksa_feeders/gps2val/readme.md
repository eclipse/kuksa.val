# GPS Feeder
consumes [gpsd](https://gpsd.gitlab.io/gpsd/) as datasource and pushes location to kuksa.val server.
The [`gpsd_feeder.ini`](./gpsd_feeder.ini) contains `kuksa.val` and `gpsd` configuration.

Before starting the gps feeder, you need start `kuksa.val` and `gpsd`:
```
<path to kuksa.val>/kuksa.val

gpsd -S <gpsd port> -N <gps device>
```

If you do not have a gps device, you can use your cellphone to forward gps data to `gpsd`. For example [gpsd-forward](https://github.com/tiagoshibata/Android-GPSd-Forwarder) is an open source android app. You can start gpsd with the following command to receive data from the app:

```
gpsd -N udp://0.0.0.0:29998
```

You can also use [gpsfake](https://gpsd.gitlab.io/gpsd/gpsfake.html) to playback a gps logs in e.g. nmea format.
To install `gpsfake`, follow the command in this [link](https://command-not-found.com/gpsfake).
After installation, run the following command to simulate a gps device as datasource:
```
gpsfake -P 2947 simplelog_example.nmea
```
Note: You need to use the `gpsfake` with the same version like the installed `gpsd`.

There are several tools for generating nmea log files:
- [nmea-gps logger](https://www.npmjs.com/package/nmea-gps-logger)
- [nmeagen](https://nmeagen.org/)
