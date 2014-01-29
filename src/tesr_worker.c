#include "tesr_worker.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tesr_common.h"
#include "tesr_queue.h"
#include "tesr_rate_limiter.h"
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
void init_worker(worker_thread_t *thiz, main_thread_t *main_thread, tesr_config_t *config, rate_limiter_t *rate_limiter, int idx) {
    LOG_LOC;
    thiz->idx = idx;
    thiz->main_thread = main_thread;
    thiz->filters = NULL;
    tesr_filter_t *filter = NULL;
    tesr_filter_t *cpfilter = NULL;
    LL_FOREACH(config->filters, filter) {
        LOG_DEBUG("Prepend> %s\n", filter->filter);
        cpfilter = (tesr_filter_t*)malloc(sizeof(tesr_filter_t));
        cpfilter = memcpy(cpfilter, filter, sizeof(tesr_filter_t));
        LL_PREPEND(thiz->filters, cpfilter);
    }
    thiz->rate_limiter = rate_limiter;
    thiz->queue = create_queue();
    init_queue(thiz->queue);
    connect_pipe(&thiz->int_fd, &thiz->ext_fd);
    thiz->event_loop = ev_loop_new(0);
    ev_io_init(&thiz->inbox_watcher, inbox_cb_w, thiz->int_fd, EV_READ);
    ev_io_start(thiz->event_loop, &thiz->inbox_watcher);
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


void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    LOG_WARN("inbox_cb_w\n");
    int idx;
    size_t len = sizeof(int);
    int ret = read(w->fd, &idx, len);
    if (ret != len) {
        LOG_ERROR("Can't read from connection notify pipe\n");
        LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
    } else {
        worker_thread_t *worker_thread = get_worker_thread(idx);
        if(worker_thread) {
            queue_data_t *data = tesr_dequeue(worker_thread->queue);
            if(data) {
                if(should_echo(data->buffer, data->bytes, &data->addr, worker_thread->filters, worker_thread->rate_limiter)) {
                    size_t len = sizeof(data->worker_idx);
                    LOG_DEBUG("[OK]>thread = 0x%zx should_echo\n", (size_t)pthread_self());
                    tesr_enqueue(worker_thread->main_thread->queue, data);
                    LOG_DEBUG("starting blocking write on thread = 0x%zx\n", (size_t)pthread_self());
                    if (write(worker_thread->main_thread->ext_fd, &data->worker_idx, len) != len) {
                        LOG_ERROR("Fail to writing to connection notify pipe\n");
                    }
                } else {
                    LOG_DEBUG("[KO]Xthread = 0x%zx should_NOT_echo\n", (size_t)pthread_self());
                }
            }



            //work_on_queue(worker_thread->queue, worker_thread->main_thread->queue, worker_thread->filters, worker_thread->rate_limiter);
            //pthread_mutex_lock(&worker_thread->lock);     //Don't forget locking
            //process_worker_data(worker_thread->queue, worker_thread->main_thread, worker_thread->filters, worker_thread->rate_limiter, idx);
            //pthread_mutex_unlock(&worker_thread->lock);   //Don't forget unlocking
        }
    }
}

