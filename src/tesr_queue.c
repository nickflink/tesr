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

static int tesr_lock(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    int lock_error = pthread_mutex_trylock(mutex);     //Don't forget locking
    //LOG_DEBUG("[0x%zx] REQUESTED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    while(lock_error != 0) {
        LOG_DEBUG("[0x%zx] FAILURE (%s) by thread = 0x%zx\n", (size_t)mutex, get_lock_error_string(lock_error), (size_t)pthread_self());
        pthread_cond_wait(cond, mutex);
        lock_error = pthread_mutex_trylock(mutex);     //Don't forget locking
    }
    //LOG_DEBUG("[0x%zx] AQUIRED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    return lock_error;
}

static int tesr_unlock(pthread_mutex_t *mutex, pthread_cond_t *cond) {
    //LOG_DEBUG("[0x%zx] PENDING by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    int lock_error = pthread_mutex_unlock(mutex);   //Don't forget unlocking
    if(lock_error == 0) {
        pthread_cond_broadcast(cond);
        //LOG_DEBUG("[0x%zx] RELEASED by thread = 0x%zx\n", (size_t)mutex, (size_t)pthread_self());
    } else {
        LOG_DEBUG("[0x%zx] FAILURE (%s) by thread = 0x%zx\n", (size_t)mutex, get_lock_error_string(lock_error), (size_t)pthread_self());
    }
    return lock_error;
}

void tesr_enqueue(tesr_queue_t *thiz, queue_data_t *data) {
    LOG_DEBUG("tesr_enqueue thread = 0x%zx\n", (size_t)pthread_self());
    tesr_lock(&thiz->mutex, &thiz->cond);  //Don't forget unlocking
    LL_APPEND(thiz->queue, data);
    tesr_unlock(&thiz->mutex, &thiz->cond);   //Don't forget unlocking
}

queue_data_t *tesr_dequeue(tesr_queue_t *thiz) {
    LOG_DEBUG("tesr_dequeue thread = 0x%zx\n", (size_t)pthread_self());
    queue_data_t *data = NULL;
    //LOG_DEBUG("starting blocking read on thread = 0x%zx\n", (size_t)pthread_self());
    tesr_lock(&thiz->mutex, &thiz->cond);  //Don't forget unlocking
    data = thiz->queue;
    LL_DELETE(thiz->queue, data);
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
