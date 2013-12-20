#ifndef TESR_CONFIG_H
#define TESR_CONFIG_H
#define DEFAULT_PORT 7
#include <arpa/inet.h>
#include <tesr_common.h>
typedef struct tesr_send_port_t {
    int port;
    struct tesr_send_port_t *next; /* needed for singly- or doubly-linked lists */
} tesr_send_port_t;
typedef struct tesr_config_t {
    int recv_port;
    int ip_rate_limit_max;
    int ip_rate_limit_period;
    int num_worker_threads;
    tesr_send_port_t *send_ports;
    tesr_filter_t *filters;
} tesr_config_t;
void log_config(tesr_config_t *tesr_config);
void init_config(tesr_config_t *tesr_config, int argc, char **argv);
#endif //TESR_CONFIG_H
