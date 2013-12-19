#include "tesr_worker.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <tesr_common.h>
#include <unistd.h> // for usleep
#include <utlist.h>

//worker_thread_t *me = NULL;
static worker_thread_t *worker_threads = NULL;
static int num_threads = 0;
static worker_thread_t *get_worker_thread(int idx) {
    worker_thread_t *worker_thread = NULL;
    if(worker_threads && 0 <= idx && idx < num_threads) {
        worker_thread = &worker_threads[idx];
    }
    return worker_thread;
}

worker_thread_t *create_workers(int num) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    if(worker_threads) {
        perror("cant create workers more than once");
    }
    num_threads = num;
    worker_threads = (worker_thread_t*)malloc(num*sizeof(worker_thread_t));
    return worker_threads;
}
void* worker_thread_start(void* args) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    printf("worker_thread_start=%d\n", (int)pthread_self());
    worker_thread_t *me = (worker_thread_t*)args;
    log_worker(me);
    printf("> %s::%s::%d\n", __FILE__, __FUNCTION__, __LINE__);
    int ret = bind_dgram_socket(&me->sd, &me->addr, me->port);
    printf("> %s::%s::%d\n", __FILE__, __FUNCTION__, __LINE__);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }
    ev_loop(me->event_loop, 0);
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    return NULL;
}

//called on the main thread
void init_worker(worker_thread_t *worker_thread, int port, int idx) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    worker_thread->idx = idx;
    pthread_mutex_init(&worker_thread->lock, NULL);
    // This loop sits in the pthread
    worker_thread->port = port;
#ifdef USE_PIPES
    int fds[2];
    if(pipe(fds)) {
        perror("Can't create notify pipe");
        return;
    }
    worker_thread->inbox_fd = fds[0];
    worker_thread->outbox_fd = fds[1];
    worker_thread->event_loop = ev_loop_new(0);
    ev_io_init(&worker_thread->inbox_watcher, inbox_cb_w, worker_thread->inbox_fd, EV_READ);
    ev_io_start(worker_thread->event_loop, &worker_thread->inbox_watcher);
#else //!USE_PIPES
    worker_thread->event_loop = ev_loop_new(0);
    ev_async_init(&worker_thread->async_watcher, async_echo_cb);
    ev_async_start(worker_thread->event_loop, &worker_thread->async_watcher);
#endif
}
void log_worker(worker_thread_t *worker_thread) {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    printf("pthread_self[%d][%d]worker_thread.pthread\n\tport=%d\n",(int)pthread_self(), (int)worker_thread->thread, worker_thread->port);
}
void destroy_workers() {
    printf("> %s::%s\n", __FILE__, __FUNCTION__);
    if(worker_threads) {
        free(worker_threads);
        num_threads = 0;
        worker_threads = NULL;
    }
}


#ifdef USE_PIPES
void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    int idx;
    //printf("pthread = %d readable\n", (int)pthread_self());
    size_t len = sizeof(int);
//check the size
//fseek(w->fd, 0L, SEEK_END);
//int sz = ftell(w->fd);
//fseek(w->fd, 0L, SEEK_SET);
struct stat buf;
fstat(w->fd, &buf);
int sz = buf.st_size;
    int ret = read(w->fd, &idx, len);
    if (ret != len) {
        perror("Can't read from connection notify pipe\n");
        printf("[KO] ret = %d != %d len sz = %d\n", ret, (int)len, sz);
    } else {
        worker_thread_t *worker_thread = get_worker_thread(idx);
        if(worker_thread) {
            pthread_mutex_lock(&worker_thread->lock);     //Don't forget locking
            worker_data_t *worker_data = worker_thread->queue;
            if(worker_data) {
                printf("[OK] ret = %d == %d len sz = %d\n", ret, (int)len, sz);
                static int send_count = 0;
                printf("thread = %d send_count %d\n", (int)pthread_self(), send_count++);
                //printf("pthread = %d readable buffer %s\n", (int)pthread_self(), data.buffer);
                //usleep(100);
                if(should_echo(worker_data->buffer, worker_data->bytes)) {
                    sendto(worker_thread->sd, worker_data->buffer, worker_data->bytes, 0, (struct sockaddr*) &worker_data->addr, sizeof(worker_data->addr));
                }
                LL_DELETE(worker_thread->queue,worker_data);
            }
            pthread_mutex_unlock(&worker_thread->lock);   //Don't forget unlocking
        }
    }
}
#else //!USE_PIPES
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
#endif //USE_PIPES

