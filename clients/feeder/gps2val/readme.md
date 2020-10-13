# GPS Feeder
consumes gpsd as datasource and pushes location to kuksa.val server

For testing, you can use [gpsfake](https://gpsd.gitlab.io/gpsd/gpsfake.html) to playback a gps logs in e.g. nmea format.
```
gpsfake -P 2947 simplelog_example.nmea
```

There are several tools for generating nmea log files:
- [nmea-gps logger](https://www.npmjs.com/package/nmea-gps-logger)
- [nmeagen](https://nmeagen.org/)
