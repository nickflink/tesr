#ifndef TESR_CONFIG_H
#define TESR_CONFIG_H
#define DEFAULT_PORT 7
#include <arpa/inet.h>

typedef struct tesr_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct tesr_filter_t *next; /* needed for singly- or doubly-linked lists */
} tesr_filter_t;

typedef struct tesr_config_t {
    int port;
    tesr_filter_t *filters;
} tesr_config_t;
void log_config(tesr_config_t *tesr_config);
void init_config(tesr_config_t *tesr_config, int argc, char **argv);
#endif //TESR_CONFIG_H
