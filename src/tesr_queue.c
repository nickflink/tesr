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
    int fds[2];
    if(pipe(fds)) {
        LOG_ERROR("Can't create notify pipe");
        return 0;
    }
    thiz->int_fd = fds[0];
    thiz->ext_fd = fds[1];
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

void tesr_enqueue(tesr_queue_t *thiz, queue_data_t *data) {
    LOG_DEBUG("tesr_enqueue thread = 0x%zx\n", (size_t)pthread_self());
    LOG_DEBUG("[0x%zx] REQUESTED by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    int lock_error = pthread_mutex_trylock(&thiz->mutex);     //Don't forget locking
    while(lock_error != 0) {
        log_lock_error(lock_error);
        pthread_cond_wait(&thiz->cond, &thiz->mutex);
        lock_error = pthread_mutex_trylock(&thiz->mutex);     //Don't forget locking
    }
    LOG_DEBUG("[0x%zx] AQUIRED by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    LL_APPEND(thiz->queue, data);
    size_t len = sizeof(data->worker_idx);
    LOG_DEBUG("starting blocking write on thread = 0x%zx\n", (size_t)pthread_self());
    if (write(thiz->ext_fd, &data->worker_idx, len) != len) {
        LOG_ERROR("Fail to writing to connection notify pipe\n");
    }
    LOG_DEBUG("[0x%zx] PENDING by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    pthread_cond_broadcast(&thiz->cond);
    pthread_mutex_unlock(&thiz->mutex);   //Don't forget unlocking
    LOG_DEBUG("[0x%zx] RELEASED by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
}

queue_data_t *tesr_dequeue(tesr_queue_t *thiz) {
    LOG_DEBUG("tesr_dequeue thread = 0x%zx\n", (size_t)pthread_self());
    queue_data_t *data = NULL;
    int worker_idx = 0;
    size_t len = sizeof(int);
    LOG_DEBUG("LOCK [0x%zx] REQUESTED thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    int lock_error = pthread_mutex_trylock(&thiz->mutex);     //Don't forget locking
    while(lock_error != 0) {
        log_lock_error(lock_error);
        pthread_cond_wait(&thiz->cond, &thiz->mutex);
        lock_error = pthread_mutex_trylock(&thiz->mutex);     //Don't forget locking
    }
    LOG_DEBUG("LOCK [0x%zx] AQUIRED thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    LOG_DEBUG("starting blocking read on thread = 0x%zx\n", (size_t)pthread_self());
    int ret = read(thiz->int_fd, &worker_idx, len);
    if (ret != len) {
        LOG_ERROR("Can't read from connection notify pipe\n");
        LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
    } else {
        data = thiz->queue;
        LL_DELETE(thiz->queue, data);
    }
    LOG_DEBUG("LOCK [0x%zx] PENDING by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
    pthread_cond_broadcast(&thiz->cond);
    pthread_mutex_unlock(&thiz->mutex);     //Don't forget locking
    LOG_DEBUG("LOCK [0x%zx] RELEASED by thread = 0x%zx\n", (size_t)&thiz->mutex, (size_t)pthread_self());
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
