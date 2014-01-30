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

int main(int argc, char** argv) {
    LOG_LOC;
    LOG_INFO("[TID] 0x%zx %s\n", (size_t)pthread_self(), __FUNCTION__);
    tesr_config_t *config = create_config();
    init_config(config, argc, argv);
    log_config(config);

    supervisor_thread_t *supervisor = create_supervisor_instance();
    if(init_supervisor(supervisor, config)) {
        supervisor_thread_run(supervisor);
    }
    destroy_supervisor(supervisor);
    LOG_DEBUG("That's all folks!\n");
    return 0;
}
