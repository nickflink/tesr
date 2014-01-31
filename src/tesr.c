//
// Threaded Echo ServeR
// Author: Nick Flink
//

#include <arpa/inet.h>
#include <ev.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for pipe
#include <utlist.h>
#include "tesr_common.h"
#include "tesr_config.h"
#include "tesr_queue.h"
#include "tesr_rate_limiter.h"
#include "tesr_supervisor.h"
#include "tesr_types.h"
#include "tesr_worker.h"

static void daemonize(void) {
    pid_t pid, sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    LOG_INFO("prefork pid = %d\n", getpid());
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */
    LOG_INFO("daemon pid = %d\n", getpid());

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

int main(int argc, char** argv) {
    LOG_LOC;
    tesr_config_t *config = create_config();
    init_config(config, argc, argv);
    log_config(config);

    if(config->daemonize) {
        daemonize();
    }

    supervisor_thread_t *supervisor = create_supervisor_instance();
    if(init_supervisor(supervisor, config)) {
        supervisor_thread_run(supervisor);
    }
    destroy_supervisor(supervisor);
    LOG_INFO("That's all folks!\n");
    return 0;
}
