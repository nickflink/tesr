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
void init_worker(worker_thread_t *thiz, supervisor_thread_t *supervisor_thread, int idx);
void destroy_worker(worker_thread_t *thiz);
void log_worker(worker_thread_t *thiz);
void* worker_thread_run(void* args);

const char *get_thread_string();
worker_thread_t *get_worker_thread(int idx);
#endif //TESR_WORKER_H
