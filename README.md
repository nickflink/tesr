# Threaded Echo ServeR
Tools for checking udp throughput on networks

## Components
* Simple echo server with rate limiting written in c using libev
* EchoBlast client with producer consumer pattern written in java

## Features
* scalable multithreaded design using libev
* deb package creation
* configurably filter blacklisted IP RegEx's 
* ratelimit source ips over time

## Conf
[checkout the example tesr.conf](etc/tesr/tesr.conf)

## Build
```bash
./configure
make
```
## Make Args
```
make STRICT=1 (to treat warnings as errors)
make LINK_STATIC=1 (to statically link all libraries)
make LOG_LEVEL=0 (for silent builds)
make LOG_LEVEL=1 (for only errors)
make LOG_LEVEL=2 (for errors and warnings)
make LOG_LEVEL=3 (default: for errors, warnings and info)
make LOG_LEVEL=4 (for errors, warnings, info and debug)
make LOG_LEVEL=5 (for errors, warnings, info, debug and verbose traces)
```

## Run
```bash
shell#1$ ./bin/tesr -p 2007
shell#2$ java EchoBlast
```

## Args
```
USAGE tesr [T]hreaded [E]cho [S]erve[R]
-d (daemonize)
-p --port (override the port)
-w --workers (override the number of workers)
```

## Package Debian Only
*We use fpm documented here https://github.com/jordansissel/fpm/wiki*
```bash
gem install fpm
make package
```
