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
    tesr_queue_t *tesr_queue = NULL;
    tesr_queue = (tesr_queue_t*)malloc(sizeof(tesr_queue_t));
    return tesr_queue;
}
int init_queue(tesr_queue_t *thiz) {
    thiz->queue = NULL;
    pthread_mutex_init(&thiz->mutex, NULL);
    pthread_cond_init(&thiz->cond, NULL);
    return 1;
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

static int tesr_trylock(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    int lock_error = pthread_mutex_trylock(mutex);     //Don't forget locking
    LOG_DEBUG("[0x%zx] REQUESTED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    if(lock_error == 0) {
        LOG_DEBUG("[0x%zx] AQUIRED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    } else {
        log_lock_error(lock_error);
        pthread_cond_wait(cond, mutex);
    }
    return lock_error;
}

static int tesr_unlock(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    LOG_DEBUG("[0x%zx] PENDING by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    int lock_error = pthread_mutex_unlock(mutex);   //Don't forget unlocking
    if(lock_error == 0) {
        pthread_cond_broadcast(cond);
        LOG_DEBUG("[0x%zx] RELEASED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    } else {
        log_lock_error(lock_error);
    }
    return lock_error;
}

void tesr_enqueue(tesr_queue_t *thiz, queue_data_t *data) {
    LOG_DEBUG("tesr_enqueue thread = 0x%zx\n", (size_t)pthread_self());
    while(tesr_trylock(&thiz->mutex, &thiz->cond) != 0);   //Don't forget unlocking
    LL_APPEND(thiz->queue, data);
    tesr_unlock(&thiz->mutex, &thiz->cond);   //Don't forget unlocking
    //size_t len = sizeof(data->worker_idx);
    //LOG_DEBUG("starting blocking write on thread = 0x%zx\n", (size_t)pthread_self());
    //if (write(thiz->ext_fd, &data->worker_idx, len) != len) {
    //    LOG_ERROR("Fail to writing to connection notify pipe\n");
    //}
}

queue_data_t *tesr_dequeue(tesr_queue_t *thiz) {
    LOG_DEBUG("tesr_dequeue thread = 0x%zx\n", (size_t)pthread_self());
    queue_data_t *data = NULL;
    //int worker_idx = 0;
    //size_t len = sizeof(int);
    LOG_DEBUG("starting blocking read on thread = 0x%zx\n", (size_t)pthread_self());
    while(tesr_trylock(&thiz->mutex, &thiz->cond) != 0);   //Don't forget unlocking
    //int ret = read(thiz->int_fd, &worker_idx, len);
    //if (ret != len) {
    //    LOG_ERROR("Can't read from connection notify pipe\n");
    //    LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
    //} else {
        data = thiz->queue;
        LL_DELETE(thiz->queue, data);
    //}
    tesr_unlock(&thiz->mutex, &thiz->cond);   //Don't forget unlocking
    return data;
}

void destroy_queue(tesr_queue_t *thiz) {
    if(thiz) {
        free(thiz);
    } else {
        LOG_ERROR("can not free tesr_queue_t * as it is NULL");
    }
}

queue_data_t *create_queue_data() {
    queue_data_t *queue_data = NULL;
    queue_data = (queue_data_t*)malloc(sizeof(queue_data_t));
    return queue_data;
}

void destroy_queue_data(queue_data_t *thiz) {
    if(thiz) {
        free(thiz);
    } else {
        LOG_ERROR("can not free queue_data_t * as it is NULL");
    }
}
