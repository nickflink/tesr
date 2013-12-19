#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
typedef struct tesr_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct tesr_filter_t *next; /* needed for singly- or doubly-linked lists */
} tesr_filter_t;
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters);
#endif //TESR_COMMON_H
