#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <tesr_logging.h> //needed for logging macros

//typedef struct rate_limit_struct_t {
//    char ip[INET_ADDRSTRLEN]; /* we'll use this field as the key */
//    time_t last_check;
//    int count;
//    UT_hash_handle hh;        /* makes this structure hashable */
//} rate_limit_struct_t;
typedef struct tesr_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct tesr_filter_t *next; /* needed for singly- or doubly-linked lists */
} tesr_filter_t;
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
//int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters);
int passes_filters(char *ip, tesr_filter_t *filters);
int is_correctly_formatted(char *buffer);
#endif //TESR_COMMON_H
