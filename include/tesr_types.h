#ifndef TESR_TYPES_H
#define TESR_TYPES_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <ev.h>
#include <pthread.h>
#include <tesr_logging.h> //needed for logging macros
#include <utlist.h>
#include <uthash.h>

#define BUFFER_LEN 1024

typedef struct tesr_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct tesr_filter_t *next; /* needed for singly- or doubly-linked lists */
} tesr_filter_t;

typedef struct tesr_config_t {
    int recv_port;
    int ip_rate_limit_max;
    int ip_rate_limit_period;
    int ip_rate_limit_prune_mark;
    int num_workers;
    tesr_filter_t *filters;
} tesr_config_t;

typedef struct rate_limit_struct_t {
    char ip[INET_ADDRSTRLEN]; /* we'll use this field as the key */
    time_t last_check;
    int count;
    UT_hash_handle hh;        /* makes this structure hashable */
} rate_limit_struct_t;

typedef struct rate_limiter_t {
    rate_limit_struct_t *rate_limit_map;
    pthread_mutex_t lock;
    int ip_rate_limit_max;
    int ip_rate_limit_period;
    int ip_rate_limit_prune_mark;
} rate_limiter_t;

typedef struct queue_data_t {
    char buffer[BUFFER_LEN];
    struct sockaddr_in addr;
    socklen_t bytes;
    int addr_len;
    int worker_idx;
    struct queue_data_t *next;
} queue_data_t;

typedef struct tesr_queue_t {
    queue_data_t *queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct queue_data_t *next;
} tesr_queue_t;

typedef struct worker_thread_t {
    int idx;
    int int_fd;
    int ext_fd;
    struct sockaddr_in addr;
    struct ev_loop* event_loop;
    struct ev_io inbox_watcher;
    //struct ev_signal signal_watcher;
    ev_async async_watcher;
    pthread_t thread;
    tesr_queue_t *queue;
    tesr_filter_t *filters;
    rate_limiter_t *rate_limiter;
//    supervisor_thread_t *main_thread;
} worker_thread_t;

typedef struct supervisor_thread_t {
    int sd;
    int int_fd;
    int ext_fd;
    int next_thread_idx;
    struct sockaddr_in addr;
    struct ev_loop* event_loop;
    struct ev_io udp_read_watcher;
    struct ev_io inbox_watcher;
    struct ev_signal sigint_watcher;
    struct ev_signal sigchld_watcher;
    tesr_queue_t *queue;
    rate_limiter_t *rate_limiter;
    worker_thread_t **worker_threads;
    tesr_config_t *config;
} supervisor_thread_t;

#endif //TESR_TYPES_H
