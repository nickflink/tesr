#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
#include "tesr_types.h"
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int passes_filters(char *ip, tesr_filter_t *filters);
int is_correctly_formatted(char *buffer);
#endif //TESR_COMMON_H
