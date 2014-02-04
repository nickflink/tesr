# Threaded Echo ServeR
Tools for checking udp throughput on networks

## Components
Simple echo server with rate limiting written in c using libev <br>
EchoBlast client with producer consumer pattern written in java

## Conf
```
#udp echo server
recv_port = 2007;
# how many worker to use
num_workers = 0;
# put any blacklisted ips in the below list
# this should at the very least contain each of the turn servers
filters = ["0.0.0.0"];
# the maximum packages to allow before discontinuing echo to that ip
# use ip_rate_limit_max = 0 to disable rate limiting
ip_rate_limit_max = 1000;
# the period in seconds of how long to wait before expiring the ip_rate_limit_max
ip_rate_limit_period = 10;
# the mark at which we trigger pruning of the rate_limits with expired periods
ip_rate_limit_prune_mark = 0;
```
## Build
```bash
./configure
make 
```
## Make Args
```
make LINK_STATIC (to statically link all libraries)
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
