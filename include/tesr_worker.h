#ifndef TESR_FILTER_H
#define TESR_FILTER_H
#define BUFFER_LEN 1024
#include <arpa/inet.h> //needed for sockaddr_in
#include <ev.h> //needed for ev_loop & ev_io
#include <stdio.h>
#include <pthread.h> //needed for pthread_t

typedef struct worker_data_t {
    char buffer[BUFFER_LEN];
    struct sockaddr_in addr;
    socklen_t bytes;
    int addr_len;
    struct worker_data_t *next;
} worker_data_t;

typedef struct worker_thread_t {
    int sd;
    struct sockaddr_in addr;
    int inbox_fd;
    int outbox_fd;
    pthread_t thread;
    struct ev_loop* event_loop;
    struct ev_io inbox_watcher;
    worker_data_t *queue;
    ev_async async_watcher;
    pthread_mutex_t lock;
} worker_thread_t;
worker_thread_t *new_workers(int num);
void* worker_thread_start(void* args);
void init_worker(worker_thread_t *worker_thread);
void log_worker(worker_thread_t *worker_thread);
void delete_worker(worker_thread_t *);
void async_echo_cb(EV_P_ ev_async *w, int revents);
#endif //TESR_FILTER_H
