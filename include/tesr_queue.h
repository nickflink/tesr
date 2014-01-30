#ifndef TESR_QUEUE_H
#define TESR_QUEUE_H
#define BUFFER_LEN 1024
#include <arpa/inet.h> //needed for sockaddr_in
#include <ev.h> //needed for ev_loop & ev_io
#include <stdio.h>
#include <pthread.h> //needed for pthread_t
#include <tesr_config.h>
#include <tesr_rate_limiter.h>
#include "tesr_types.h"
tesr_queue_t *create_queue();
int init_queue(tesr_queue_t *thiz);
void destroy_queue(tesr_queue_t *thiz);
void log_queue(tesr_queue_t *thiz);
void tesr_enqueue(tesr_queue_t *thiz, queue_data_t *data, const char *tname);
queue_data_t *tesr_dequeue(tesr_queue_t *thiz, const char *tname);
#endif //TESR_QUEUE_H
