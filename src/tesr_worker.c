#include "tesr_worker.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <tesr_common.h>
#include <unistd.h> // for usleep
#include <utlist.h>

worker_thread_t *me = NULL;
worker_thread_t *new_workers(int num) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    return (worker_thread_t*)malloc(num*sizeof(worker_thread_t));
}
void* worker_thread_start(void* args) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    printf("worker_thread_start=%d\n", (int)pthread_self());
    me = (worker_thread_t*)args;
    log_worker((worker_thread_t*)args);
    printf("> %s::%s::%d\n", __FILE__, __FUNCTION__, __LINE__);
    int port = 1981;
    printf("> %s::%s::%d\n", __FILE__, __FUNCTION__, __LINE__);
    int ret = bind_dgram_socket(&me->sd, &me->addr, port);
    printf("> %s::%s::%d\n", __FILE__, __FUNCTION__, __LINE__);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }
    ev_loop(me->event_loop, 0);
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    return NULL;
}
void init_worker(worker_thread_t *worker_thread) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    printf("implement %s", __FUNCTION__);
    worker_thread->event_loop = ev_loop_new(0);
}
void log_worker(worker_thread_t *worker_thread) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    printf("pthread_self[%d][%d]worker_thread.pthread\n",(int)pthread_self(), (int)worker_thread->thread);
}
void delete_worker(worker_thread_t *worker) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    free(worker);
}

void async_echo_cb(EV_P_ ev_async *w, int revents) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    pthread_mutex_lock(&me->lock);     //Don't forget locking
    worker_data_t *data, *tmp;
    LL_FOREACH_SAFE(me->queue, data, tmp) {
        sendto(me->sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
        //TODO: free data
        usleep(10);
        static int async_count = 0;
        ++async_count;
        printf("tid = %d async_cb %d\n", (int)pthread_self(), async_count);
        LL_DELETE(me->queue, data);
    }
    pthread_mutex_unlock(&me->lock);   //Don't forget unlocking
}
