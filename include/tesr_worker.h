#ifndef TESR_FILTER_H
#define TESR_FILTER_H
#define BUFFER_LEN 1024
#include <arpa/inet.h> //needed for sockaddr_in
#include <ev.h> //needed for ev_loop & ev_io
#include <stdio.h>
#include <pthread.h> //needed for pthread_t
#include <tesr_config.h>
#include <tesr_rate_limiter.h>
typedef struct worker_data_t {
    char buffer[BUFFER_LEN];
    struct sockaddr_in addr;
    socklen_t bytes;
    int addr_len;
    struct worker_data_t *next;
} worker_data_t;

typedef struct worker_thread_t {
    int idx;
    int sd;
    int port;
    struct sockaddr_in addr;
    int inbox_fd;
    int outbox_fd;
    pthread_t thread;
    struct ev_loop* event_loop;
    struct ev_io inbox_watcher;
    worker_data_t *queue;
    ev_async async_watcher;
    pthread_mutex_t lock;
    tesr_filter_t *filters;
    rate_limiter_t *rate_limiter;
} worker_thread_t;
worker_thread_t *create_workers(int num);
void* worker_thread_start(void* args);
void init_worker(worker_thread_t *worker_thread, tesr_config_t *config, rate_limiter_t *rate_limiter, int port, int idx);
void log_worker(worker_thread_t *worker_thread);
void destroy_workers();
void inbox_cb_w(EV_P_ ev_io *w, int revents);
#endif //TESR_FILTER_H
