//
// Threaded Echo ServeR
// Author: Nick Flink
//

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
#include "tesr_rate_limiter.h"
#include "tesr_types.h"
#include "tesr_worker.h"
main_thread_t main_thread;
int next_thread_idx = 0;
worker_thread_t *worker_threads;
tesr_config_t tesr_config;

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    int th = next_thread_idx;
    worker_data_t *data = ((worker_data_t *)malloc(sizeof(worker_data_t)));
    data->addr_len = sizeof(struct sockaddr_in);
    data->bytes = recvfrom(main_thread.sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
    if(tesr_config.num_workers == 0) {
        //sendto(main_thread.sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
        process_worker_data(data, &main_thread, tesr_config.filters, main_thread.rate_limiter, th);
    } else {
        pthread_mutex_lock(&worker_threads[th].lock);     //Don't forget locking
        LL_APPEND(worker_threads[th].queue, data);
        static int recv_count = 0;
        ++recv_count;
        LOG_DEBUG("[OK]<thread = 0x%zx recv_count %d\n", (size_t)pthread_self(), recv_count);
        size_t len = sizeof(th);
        if (write(worker_threads[th].ext_fd, &th, len) != len) {
            LOG_ERROR("Fail to writing to connection notify pipe\n");
        }
        pthread_mutex_unlock(&worker_threads[th].lock);   //Don't forget unlocking
        if(++next_thread_idx >= tesr_config.num_workers) {
            next_thread_idx = 0;
        }
    }
}

static void udp_write_cb(EV_P_ ev_io *w, int revents) {
    LOG_WARN("udp_write_cb\n");
    int idx;
    size_t len = sizeof(int);
    pthread_mutex_lock(&main_thread.lock);     //Don't forget locking
    int ret = read(w->fd, &idx, len);
    if (ret != len) {
        LOG_ERROR("Can't read from connection notify pipe\n");
        LOG_INFO("[KO] ret = %d != %d len\n", ret, (int)len);
    } else {
        worker_data_t *data = main_thread.queue;
        sendto(main_thread.sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
        LL_DELETE(main_thread.queue, data);
    }
    pthread_mutex_unlock(&main_thread.lock);     //Don't forget locking
}

int main(int argc, char** argv) {
    LOG_LOC;
    LOG_INFO("[TID] 0x%zx %s\n", (size_t)pthread_self(), __FUNCTION__);
    init_config(&tesr_config, argc, argv);
    log_config(&tesr_config);
    if(tesr_config.num_workers == 0) {
        LOG_WARN("you are running using only one thread (num_workers=0)\n");
    }
    int ret = bind_dgram_socket(&main_thread.sd, &main_thread.addr, tesr_config.recv_port);
    if (ret == 0) {
        LOG_ERROR("could not bind dgram socket");
        return 0;
    }

    pthread_mutex_init(&main_thread.lock, NULL);
    // This loop sits in the pthread
    int fds[2];
    if(pipe(fds)) {
        LOG_ERROR("Can't create notify pipe");
        return 0;
    }
    main_thread.int_fd = fds[0];
    main_thread.ext_fd = fds[1];

    main_thread.event_loop = EV_DEFAULT;  //or ev_default_loop (0);
    //Set up rate limiting
    rate_limiter_t *rate_limiter = create_rate_limiter();
    main_thread.rate_limiter = rate_limiter;
    init_rate_limiter(rate_limiter, tesr_config.ip_rate_limit_max, tesr_config.ip_rate_limit_period, tesr_config.ip_rate_limit_prune_mark);

    //Initialize pthread
    worker_threads = create_workers(tesr_config.num_workers);
    int th=0;
    for(th=0; th < tesr_config.num_workers; th++) {
        init_worker(&worker_threads[th], &main_thread, &tesr_config, rate_limiter, th);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&worker_threads[th].thread, &attr, worker_thread_start, &worker_threads[th]);
        ++th;
    }

    ev_io_init(&main_thread.udp_read_watcher, udp_read_cb, main_thread.sd, EV_READ);
    ev_io_start(main_thread.event_loop, &main_thread.udp_read_watcher);
    ev_io_init(&main_thread.inbox_watcher, udp_write_cb, main_thread.int_fd, EV_READ);
    ev_io_start(main_thread.event_loop, &main_thread.inbox_watcher);

    // now wait for events to arrive
    ev_loop(main_thread.event_loop, 0);
    //Wait on threads for execution
    for(th = 0; th < tesr_config.num_workers; th++) {
        pthread_join(worker_threads[th].thread, NULL);
        pthread_mutex_destroy(&worker_threads[th].lock);
    }
    return 0;
}
