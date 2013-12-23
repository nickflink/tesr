#include "tesr_worker.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tesr_common.h>
#include <tesr_rate_limiter.h>
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
    LOG_LOC;
    if(worker_threads) {
        LOG_ERROR("cant create workers more than once");
    }
    num_threads = num;
    worker_threads = (worker_thread_t*)malloc(num*sizeof(worker_thread_t));
    return worker_threads;
}
void* worker_thread_start(void* args) {
    LOG_LOC;
    LOG_INFO("[TID] 0x%zx %s\n", (size_t)pthread_self(), __FUNCTION__);
    worker_thread_t *me = (worker_thread_t*)args;
    log_worker(me);
    int ret = bind_dgram_socket(&me->sd, &me->addr, me->port);
    if (ret != 0) {
        LOG_ERROR("could not bind dgram socket");
    }
    ev_loop(me->event_loop, 0);
    return NULL;
}

//called on the main thread
void init_worker(worker_thread_t *worker_thread, tesr_config_t *config, rate_limiter_t *rate_limiter, int port, int idx) {
    LOG_LOC;
    worker_thread->idx = idx;
    worker_thread->filters = NULL;
    tesr_filter_t *filter = NULL;
    tesr_filter_t *cpfilter = NULL;
    LL_FOREACH(config->filters, filter) {
        LOG_DEBUG("Prepend> %s\n", filter->filter);
        cpfilter = (tesr_filter_t*)malloc(sizeof(tesr_filter_t));
        cpfilter = memcpy(cpfilter, filter, sizeof(tesr_filter_t));
        LL_PREPEND(worker_thread->filters, cpfilter);
    }
    worker_thread->rate_limiter = rate_limiter;
    pthread_mutex_init(&worker_thread->lock, NULL);
    // This loop sits in the pthread
    worker_thread->port = port;
#ifdef USE_PIPES
    int fds[2];
    if(pipe(fds)) {
        LOG_ERROR("Can't create notify pipe");
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
    LOG_LOC;
    LOG_INFO("worker_thread[0x%zx]{port=%d}\n",(size_t)pthread_self(), worker_thread->port);
}
void destroy_workers() {
    LOG_LOC;
    if(worker_threads) {
        free(worker_threads);
        num_threads = 0;
        worker_threads = NULL;
    }
}

static int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters, rate_limiter_t *rate_limiter) {
    int ret = 0;
    if(buffer != NULL) {
        buffer[bytes] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(addr->sin_family, &addr->sin_addr, ip, INET_ADDRSTRLEN);
        LOG_DEBUG("should_echo:%s from:%s:%d\n", buffer, ip, ntohs(addr->sin_port));
        if(is_correctly_formatted(buffer)) {
            if(passes_filters(ip, filters)) {
                if(is_under_rate_limit(rate_limiter, ip)) {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}

#ifdef USE_PIPES
void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    int idx;
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
        LOG_ERROR("Can't read from connection notify pipe\n");
        LOG_INFO("[KO] ret = %d != %d len sz = %d\n", ret, (int)len, sz);
    } else {
        worker_thread_t *worker_thread = get_worker_thread(idx);
        if(worker_thread) {
            pthread_mutex_lock(&worker_thread->lock);     //Don't forget locking
            worker_data_t *worker_data = worker_thread->queue;
            if(worker_data) {
                static int send_count = 0;
                if(should_echo(worker_data->buffer, worker_data->bytes, &worker_data->addr, worker_thread->filters, worker_thread->rate_limiter)) {
                    ++send_count;
                    LOG_DEBUG("[OK]>thread = %d send_count %d\n", (int)pthread_self(), send_count);
                    sendto(worker_thread->sd, worker_data->buffer, worker_data->bytes, 0, (struct sockaddr*) &worker_data->addr, sizeof(worker_data->addr));
                } else {
                    LOG_DEBUG("[KO]Xthread = %d send_count %d\n", (int)pthread_self(), send_count);
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

