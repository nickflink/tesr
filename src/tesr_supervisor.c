#include "tesr_supervisor.h"
#include <arpa/inet.h>
#include <ev.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for pipe
#include <utlist.h>
#include "tesr_common.h"
#include "tesr_config.h"
#include "tesr_queue.h"
#include "tesr_rate_limiter.h"
#include "tesr_types.h"
#include "tesr_worker.h"
//supervisor_thread_t main_thread;
//int next_thread_idx = 0;
//worker_thread_t *worker_threads;
//tesr_config_t tesr_config;
static supervisor_thread_t *supervisor_thread_instance = NULL;

supervisor_thread_t *get_supervisor_thread() {
    return supervisor_thread_instance;
}

static void sigint_cb (struct ev_loop *loop, struct ev_signal *w, int revents) {
    LOG_DEBUG("caught SIGINT!\n");
    supervisor_thread_t *thiz = get_supervisor_thread();
    if(thiz) {
        int kill_pill = -1;
        size_t len = sizeof(kill_pill);
        ev_io_stop(thiz->event_loop, &thiz->udp_read_watcher);
        int th = 0;
        for(th = 0; th < thiz->config->num_workers; th++) {
            if (write(thiz->worker_threads[th]->ext_fd, &kill_pill, len) != len) {
                LOG_ERROR("Fail to writing to connection notify pipe\n");
            }
            pthread_join(thiz->worker_threads[th]->thread, NULL);
            LOG_ERROR("{%s} JOINED\n", get_thread_string());
        }
        //ev_unloop(supervisor_thread->event_loop, EVUNLOOP_ONE);
        ev_unloop(thiz->event_loop, EVUNLOOP_ALL);
    }
}

static void sigchld_cb(struct ev_loop *loop, struct ev_signal *w, int revents) {
  LOG_DEBUG("caught SIGCHLD!\n");
  //ev_unloop(main_thread.event_loop, EVUNLOOP_ONE);
  //ev_unloop(main_thread.event_loop, EVUNLOOP_ALL);
}

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    supervisor_thread_t *thiz = get_supervisor_thread();
    if(thiz) {
        int th = thiz->next_thread_idx;
        queue_data_t *data = create_queue_data();
        //TODO(nick): this should be in init_queue_data
        data->addr_len = sizeof(struct sockaddr_in);
        data->bytes = recvfrom(thiz->sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
        data->worker_idx = th;
        size_t len = sizeof(data->worker_idx);
        if(thiz->config->num_workers == 0) {
            if(should_echo(data->buffer, data->bytes, &data->addr, thiz->config->filters, thiz->rate_limiter)) {
                tesr_enqueue(thiz->queue, data, get_thread_string());
                LOG_DEBUG("{%s} SENDING NOTIFY MainThread => MainThread\n", get_thread_string());
                if (write(thiz->ext_fd, &data->worker_idx, len) != len) {
                    LOG_ERROR("Fail to writing to connection notify pipe\n");
                }
            }
        } else {
            tesr_enqueue(thiz->worker_threads[th]->queue, data, get_thread_string());
            LOG_DEBUG("{%s} SENDING NOTIFY MainThread => WorkThread[%d]\n", get_thread_string(), th);
            if (write(thiz->worker_threads[th]->ext_fd, &data->worker_idx, len) != len) {
                LOG_ERROR("Fail to writing to connection notify pipe\n");
            }
            if(++thiz->next_thread_idx >= thiz->config->num_workers) {
                thiz->next_thread_idx = 0;
            }
        }
    }
}

static void udp_write_cb(EV_P_ ev_io *w, int revents) {
    LOG_LOC;
    int idx;
    size_t len = sizeof(int);
    supervisor_thread_t *thiz = get_supervisor_thread();
    if(thiz) {
        LOG_DEBUG("{%s} RECVING NOTIFY WorkThread[?] => MainThread\n", get_thread_string());
        int ret = read(w->fd, &idx, len);
        if (ret != len) {
            LOG_ERROR("Can't read from connection notify pipe\n");
            LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
        } else {
            LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
            queue_data_t *data = tesr_dequeue(thiz->queue, get_thread_string());
            LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
            sendto(thiz->sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
        }
    }
}

supervisor_thread_t *create_supervisor_instance() {
    supervisor_thread_t *thiz = NULL;
    thiz = (supervisor_thread_t*)malloc(sizeof(supervisor_thread_t));
    TESR_LOG_ALLOC(thiz, supervisor_thread_t);
    supervisor_thread_instance = thiz;
    return thiz;
}

int init_supervisor(supervisor_thread_t *thiz, tesr_config_t *config) {
    LOG_LOC;
    int ret = 0;
    if(thiz && config) {
        thiz->config = config;
        if(thiz->config->num_workers == 0) {
            LOG_WARN("you are running in single threaded mode (num_workers=0)\n");
        }
        ret = bind_dgram_socket(&thiz->sd, &thiz->addr, thiz->config->recv_port);
        if (ret == 0) {
            LOG_ERROR("could not bind dgram socket");
        } else {
            thiz->queue = create_queue();
            init_queue(thiz->queue);
            connect_pipe(&thiz->int_fd, &thiz->ext_fd);

            thiz->event_loop = EV_DEFAULT;  //or ev_default_loop (0);
            //Set up rate limiting
            thiz->rate_limiter = create_rate_limiter();
            init_rate_limiter(thiz->rate_limiter, thiz->config->ip_rate_limit_max, thiz->config->ip_rate_limit_period, thiz->config->ip_rate_limit_prune_mark);

            //Initialize pthread
            thiz->next_thread_idx = 0;
            thiz->worker_threads = create_workers(thiz->config->num_workers);
            //We must initialize workers last as we pass the supervisor data to them
            int th=0;
            for(th=0; th < thiz->config->num_workers; th++) {
                thiz->worker_threads[th] = create_worker();
                init_worker(thiz->worker_threads[th], thiz, th);
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_create(&thiz->worker_threads[th]->thread, &attr, worker_thread_run, thiz->worker_threads[th]);
            }
            ev_io_init(&thiz->udp_read_watcher, udp_read_cb, thiz->sd, EV_READ);
            ev_io_init(&thiz->inbox_watcher, udp_write_cb, thiz->int_fd, EV_READ);
            ev_signal_init(&thiz->sigint_watcher, sigint_cb, SIGINT);
            ev_signal_init(&thiz->sigchld_watcher, sigchld_cb, SIGCHLD);
            
        }
    }
    return ret;
}

void destroy_supervisor(supervisor_thread_t *thiz) {
    if(thiz) {
        int w = 0;
        for(w = 0; w < thiz->config->num_workers; w++) {
            destroy_worker(thiz->worker_threads[w]);
        }
        thiz->worker_threads = NULL;
        destroy_rate_limiter(thiz->rate_limiter);
        destroy_queue(thiz->queue);
        destroy_config(thiz->config);
        destroy_workers();
        TESR_LOG_FREE(thiz, supervisor_thread_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free supervisor_thread_t* as it is NULL");
    }
}

void log_supervisor(supervisor_thread_t *thiz) {
    LOG_INFO("supervisor_thread_t => 0x%zx\n", (size_t)pthread_self());
}

void supervisor_thread_run(supervisor_thread_t *thiz) {
    LOG_LOC;
    ev_io_start(thiz->event_loop, &thiz->udp_read_watcher);
    ev_io_start(thiz->event_loop, &thiz->inbox_watcher);
    ev_signal_start(thiz->event_loop, &thiz->sigint_watcher);
    ev_signal_start(thiz->event_loop, &thiz->sigchld_watcher);
    // now wait for events to arrive
    ev_loop(thiz->event_loop, 0);
    return;
}
