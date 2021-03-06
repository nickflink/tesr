#include "tesr_queue.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tesr_common.h>
#include <tesr_rate_limiter.h>
#include <unistd.h> // for usleep
#include <utlist.h>

tesr_queue_t *create_queue() {
    tesr_queue_t *thiz = NULL;
    thiz = (tesr_queue_t*)malloc(sizeof(tesr_queue_t));
    TESR_LOG_ALLOC(thiz, tesr_queue_t);
    return thiz;
}
int init_queue(tesr_queue_t *thiz) {
    thiz->queue = NULL;
    pthread_mutex_init(&thiz->mutex, NULL);
    return 1;
}

void destroy_queue(tesr_queue_t *thiz) {
    if(thiz) {
        TESR_LOG_FREE(thiz, tesr_queue_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free tesr_queue_t * as it is NULL");
    }
}

void log_queue(tesr_queue_t *thiz) {
    int print_comma = 0;
    LOG_INFO("filters=[");
    queue_data_t *data = NULL;
    LL_FOREACH(thiz->queue, data) {
        if(print_comma) {
            LOG_INFO(",");
        } else {
            print_comma = 1;
        }
        LOG_INFO("%s", data->buffer);
    }
    LOG_INFO("]\n");
}

static int tesr_lock(pthread_mutex_t *mutex, const char *tname) {
    int lock_error = pthread_mutex_lock(mutex);
    LOG_DEBUG("{%s} [0x%zx] REQUESTED\n", tname, (size_t)mutex);
    if(lock_error == 0) {
        LOG_DEBUG("{%s} [0x%zx] AQUIRED\n", tname, (size_t)mutex);
    } else {
        LOG_ERROR("{%s} [0x%zx] FAILURE (%s)\n", tname, (size_t)mutex, get_lock_error_string(lock_error));
    }
    return lock_error;
}

static int tesr_unlock(pthread_mutex_t *mutex, const char *tname) {
    LOG_DEBUG("{%s} [0x%zx] PENDING\n", tname, (size_t)mutex);
    int lock_error = pthread_mutex_unlock(mutex);
    if(lock_error == 0) {
        LOG_DEBUG("{%s} [0x%zx] RELEASED\n", tname, (size_t)mutex);
    } else {
        LOG_DEBUG("{%s} [0x%zx] FAILURE (%s)\n", tname, (size_t)mutex, get_lock_error_string(lock_error));
    }
    return lock_error;
}

void tesr_enqueue(tesr_queue_t *thiz, queue_data_t *data, const char *tname) {
    LOG_DEBUG("{%s} tesr_enqueue\n", tname);
    tesr_lock(&thiz->mutex, tname);  //Don't forget unlocking
    LL_APPEND(thiz->queue, data);
    tesr_unlock(&thiz->mutex, tname);   //Don't forget unlocking
}

queue_data_t *tesr_dequeue(tesr_queue_t *thiz, const char *tname) {
    LOG_DEBUG("{%s} tesr_dequeue\n", tname);
    queue_data_t *data = NULL;
    //LOG_DEBUG("starting blocking read on thread = 0x%zx\n", (size_t)pthread_self());
    tesr_lock(&thiz->mutex, tname);  //Don't forget unlocking
    data = thiz->queue;
    LL_DELETE(thiz->queue, data);
    tesr_unlock(&thiz->mutex, tname);   //Don't forget unlocking
    return data;
}


