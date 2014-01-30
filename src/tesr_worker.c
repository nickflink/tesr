#include "tesr_worker.h"

#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tesr_common.h"
#include "tesr_queue.h"
#include "tesr_rate_limiter.h"
#include "tesr_supervisor.h"
#include <unistd.h> // for usleep
#include <utlist.h>

//worker_thread_t *me = NULL;
static worker_thread_t *worker_threads = NULL;
static int num_threads = 0;

worker_thread_t *get_worker_thread(int idx) {
    worker_thread_t *worker_thread = NULL;
    if(worker_threads && 0 <= idx && idx < num_threads) {
        worker_thread = &worker_threads[idx];
    }
    return worker_thread;
}

static void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    LOG_LOC;
    int idx;
    size_t len = sizeof(int);
    supervisor_thread_t *supervisor_thread = get_supervisor_thread();
    if(supervisor_thread) {
        LOG_DEBUG("{%s} RECVING NOTIFY MainThread => WorkThread[?]\n", get_thread_string());
        int ret = read(w->fd, &idx, len);
        if (ret != len) {
            LOG_ERROR("Can't read from connection notify pipe\n");
            LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
        } if(idx < 0) {
            //ev_unloop(EV_A_, EVUNLOOP_ALL);
            LOG_DEBUG("{%s} RECVING KILL_PILL MainThread => WorkThread[?]\n", get_thread_string());
            ev_unloop(EV_A_ EVUNLOOP_ALL);
        } else {
            worker_thread_t *worker_thread = get_worker_thread(idx);
            if(worker_thread) {
                queue_data_t *data = tesr_dequeue(worker_thread->queue, get_thread_string());
                if(data) {
                    if(should_echo(data->buffer, data->bytes, &data->addr, worker_thread->filters, worker_thread->rate_limiter)) {
                        size_t len = sizeof(data->worker_idx);
                        LOG_DEBUG("[OK]>thread = 0x%zx should_echo\n", (size_t)pthread_self());
                        tesr_enqueue(supervisor_thread->queue, data, get_thread_string());
                        LOG_DEBUG("{%s} SENDING NOTIFY WorkThread[%d] => MainThread\n", get_thread_string(), idx);
                        if (write(supervisor_thread->ext_fd, &data->worker_idx, len) != len) {
                            LOG_ERROR("Fail to writing to connection notify pipe\n");
                        }
                    } else {
                        LOG_DEBUG("[KO]Xthread = 0x%zx should_NOT_echo\n", (size_t)pthread_self());
                    }
                }
            }
        }
    }
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
void* worker_thread_run(void* args) {
    LOG_LOC;
    LOG_INFO("[TID] 0x%zx %s\n", (size_t)pthread_self(), __FUNCTION__);
    worker_thread_t *thiz = (worker_thread_t*)args;
    log_worker(thiz);
    ev_io_start(thiz->event_loop, &thiz->inbox_watcher);
    ev_loop(thiz->event_loop, 0);
    return NULL;
}

//called on the main thread
void init_worker(worker_thread_t *thiz, supervisor_thread_t *supervisor_thread, int idx) {
    LOG_LOC;
    thiz->idx = idx;
    //thiz->main_thread = main_thread;
    thiz->filters = NULL;
    tesr_filter_t *filter = NULL;
    tesr_filter_t *cpfilter = NULL;
    //TODO: This should probably live in the ratelimiter
    LL_FOREACH(supervisor_thread->config->filters, filter) {
        LOG_DEBUG("Prepend> %s\n", filter->filter);
        cpfilter = (tesr_filter_t*)malloc(sizeof(tesr_filter_t));
        cpfilter = memcpy(cpfilter, filter, sizeof(tesr_filter_t));
        LL_PREPEND(thiz->filters, cpfilter);
    }
    thiz->rate_limiter = supervisor_thread->rate_limiter;
    thiz->queue = create_queue();
    init_queue(thiz->queue);
    connect_pipe(&thiz->int_fd, &thiz->ext_fd);
    thiz->event_loop = ev_loop_new(0);
    ev_io_init(&thiz->inbox_watcher, inbox_cb_w, thiz->int_fd, EV_READ);
}
void destroy_worker(worker_thread_t *thiz) {
    if(thiz) {
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free worker_thread_t * as it is NULL");
    }
}
void log_worker(worker_thread_t *thiz) {
    LOG_INFO("worker_thread[%d] => 0x%zx\n", thiz->idx, (size_t)pthread_self());
}
void destroy_workers() {
    LOG_LOC;
    if(worker_threads) {
        //destroy filters
        free(worker_threads);
        num_threads = 0;
        worker_threads = NULL;
    } else {
        LOG_ERROR("no workers to destroy");
    }
}

const char * get_thread_string() {
    int idx = 0;
    worker_thread_t *worker_thread = get_worker_thread(idx);
    while(worker_thread != NULL) {
        if(pthread_equal(pthread_self(), worker_thread->thread)) {
            switch(idx) {
                case 0:
                  return "_work0";
                case 1:
                  return "_work1";
                case 2:
                  return "_work2";
                case 3:
                  return "_work3";
                default:
                  return "_work?";
                break;
            }
        }
        ++idx;
        worker_thread = get_worker_thread(idx);
    }
    return "_main_";
}

