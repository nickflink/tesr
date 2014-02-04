#! /bin/bash
# tesr - this script starts and stops the tesr daemon
# description - tesr is a stun & turn server used for supporting direct \
#               voip connections even through firewalls
#
# Author: Alfred E. Heggestad <aeh@db.org>.
# Author: Nick Flink <nick@tuenti.com>.

set -e;

FUNCTION2CALL=$1;
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin;
DESC="re STUN server";
NAME=tesr;
ARGS="";
DAEMON=/usr/sbin/$NAME;
PIDFILE=/var/run/$NAME/$NAME.pid;
SCRIPTNAME=/etc/init.d/$NAME;
DAEMON_USER="tesr";
DAEMON_GROUP="tesr";
FUNCTION_LIB=/lib/lsb/init-functions
# Source function library
source $FUNCTION_LIB || { echo "can not source $FUNCTION_LIB exiting..."; exit 1; }
mkdir -p `dirname $PIDFILE` || { echo "can not create $PIDFILE exiting..."; exit 1; }

# Gracefully exit if the package has been removed.
test -x $DAEMON || { echo "$DAEMON does not exist exiting..."; exit 1; }
#
#	Function that starts the daemon/service.
#
function start() {
    local retval=0;
    log_daemon_msg "Start $NAME"
    start-stop-daemon --start --make-pidfile --pidfile $PIDFILE \
        --chuid=$DAEMON_USER:$DAEMON_GROUP --name tesr --startas $DAEMON -b ||
        echo -n " already running"
    retval=$?
    log_end_msg $retval
    return $retval
}

#
#	Function that stops the daemon/service.
#
function stop() {
    local retval=0
    log_daemon_msg "Stop $NAME"
    start-stop-daemon --stop --quiet --exec $DAEMON || echo -n " not running"
    retval=$?
    log_end_msg $retval
    echo
    [ $retval -eq 0 ] && rm -f $PIDFILE
    return $retval
}

#
#	Function that restarts the daemon/service.
#
function restart() {
    stop
    start
}

#
#	Function that returns stats for the daemon/service.
#
function status() {
    log_daemon_msg "Status $NAME"
    if [ -f $PIDFILE ]; then
        local pid="`cat $PIDFILE`"
        local proc_line="`ps aux|grep -E \"^${DAEMON_USER}\s*${pid}\"`"
        if [ "${proc_line}" == "" ]; then
            echo " ${pid} has exited unexpectedly"
        else
            echo " running as pid ${pid}"
        fi
    else
        echo " stopped";
    fi
}

#
#	Function that sends a SIGHUP to the daemon/service.
#
function reload() {
    start-stop-daemon --stop --quiet --pidfile $PIDFILE --name $NAME --signal 1
}

case "$FUNCTION2CALL" in
    start|stop|restart|status|reload)
        $FUNCTION2CALL
        ;;
    *)
        echo $"Usage: $0 {start|stop|status|restart|reload}"
        exit 2
esac
