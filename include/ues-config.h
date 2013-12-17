#ifndef UES_CONFIG_H
#define UES_CONFIG_H
#define DEFAULT_PORT 7
#include <arpa/inet.h>

typedef struct ues_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct ues_filter_t *next; /* needed for singly- or doubly-linked lists */
} ues_filter_t;

typedef struct ues_config_t {
    int port;
    ues_filter_t *filters;
} ues_config_t;
void log_config(ues_config_t *ues_config);
void init_config(ues_config_t *ues_config, int argc, char **argv);
#endif //UES_CONFIG_H
