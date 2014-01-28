#ifndef TESR_WORKER_H
#define TESR_WORKER_H
#include <arpa/inet.h> //needed for sockaddr_in
#include <ev.h> //needed for ev_loop & ev_io
#include <stdio.h>
#include <pthread.h> //needed for pthread_t
#include <tesr_config.h>
#include <tesr_rate_limiter.h>
#include "tesr_types.h"
worker_thread_t *create_workers(int num);
void* worker_thread_start(void* args);
void init_worker(worker_thread_t *worker_thread, main_thread_t *main_thread, tesr_config_t *config, rate_limiter_t *rate_limiter, int idx);
void log_worker(worker_thread_t *worker_thread);
void destroy_workers();
void process_worker_data(worker_data_t *worker_data, main_thread_t *main_thread, tesr_filter_t *filters, rate_limiter_t *rate_limiter, int th);
void inbox_cb_w(EV_P_ ev_io *w, int revents);
#endif //TESR_WORKER_H
