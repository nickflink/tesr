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
    if(worker_threads == NULL) {
        num_threads = num;
        worker_threads = (worker_thread_t*)malloc(num*sizeof(worker_thread_t));
    } else {
        LOG_ERROR("cant create workers more than once");
    }
    return worker_threads;
}
void* worker_thread_start(void* args) {
    LOG_LOC;
    LOG_INFO("[TID] 0x%zx %s\n", (size_t)pthread_self(), __FUNCTION__);
    worker_thread_t *me = (worker_thread_t*)args;
    log_worker(me);
    ev_loop(me->event_loop, 0);
    return NULL;
}

//called on the main thread
void init_worker(worker_thread_t *worker_thread, main_thread_t *main_thread, tesr_config_t *config, rate_limiter_t *rate_limiter, int idx) {
    LOG_LOC;
    worker_thread->idx = idx;
    worker_thread->main_thread = main_thread;
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
    int fds[2];
    if(pipe(fds)) {
        LOG_ERROR("Can't create notify pipe");
        return;
    }
    worker_thread->int_fd = fds[0];
    worker_thread->ext_fd = fds[1];
    worker_thread->event_loop = ev_loop_new(0);
    ev_io_init(&worker_thread->inbox_watcher, inbox_cb_w, worker_thread->int_fd, EV_READ);
    ev_io_start(worker_thread->event_loop, &worker_thread->inbox_watcher);
}
void log_worker(worker_thread_t *worker_thread) {
    LOG_INFO("worker_thread[%d] => 0x%zx\n",worker_thread->idx, (size_t)pthread_self());
}
void destroy_workers() {
    LOG_LOC;
    if(worker_threads) {
        free(worker_threads);
        num_threads = 0;
        worker_threads = NULL;
    } else {
        LOG_ERROR("no workers to destroy");
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
//You should have worker_thread->lock or have num_workers == 0 for this function
void process_worker_data(worker_data_t *worker_data, main_thread_t *main_thread, tesr_filter_t *filters, rate_limiter_t *rate_limiter, int th) {
    if(worker_data) {
        static int send_count = 0;
        if(should_echo(worker_data->buffer, worker_data->bytes, &worker_data->addr, filters, rate_limiter)) {
            ++send_count;
            LOG_DEBUG("[OK]>thread = 0x%zx send_count %d\n", (size_t)pthread_self(), send_count);
            //sendto(worker_thread->sd, worker_data->buffer, worker_data->bytes, 0, (struct sockaddr*) &worker_data->addr, sizeof(worker_data->addr));
            pthread_mutex_lock(&main_thread->lock);     //Don't forget locking
            LL_APPEND(main_thread->queue, worker_data);
            size_t len = sizeof(th);
            if (write(main_thread->ext_fd, &th, len) != len) {
                LOG_ERROR("Fail to writing to connection notify pipe\n");
            }
            pthread_mutex_unlock(&main_thread->lock);   //Don't forget unlocking
        } else {
            LOG_DEBUG("[KO]Xthread = 0x%zx send_count %d\n", (size_t)pthread_self(), send_count);
        }
        LL_DELETE(worker_data, worker_data);
    }
}

void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    int idx;
    size_t len = sizeof(int);
    int ret = read(w->fd, &idx, len);
    if (ret != len) {
        LOG_ERROR("Can't read from connection notify pipe\n");
        LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
    } else {
        worker_thread_t *worker_thread = get_worker_thread(idx);
        if(worker_thread) {
            pthread_mutex_lock(&worker_thread->lock);     //Don't forget locking
            process_worker_data(worker_thread->queue, worker_thread->main_thread, worker_thread->filters, worker_thread->rate_limiter, idx);
            pthread_mutex_unlock(&worker_thread->lock);   //Don't forget unlocking
        }
    }
}

