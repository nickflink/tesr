#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
#include "tesr_types.h"
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int passes_filters(char *ip, tesr_filter_t *filters);
int is_correctly_formatted(char *buffer);
int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters, rate_limiter_t *rate_limiter);
void log_lock_error(int lock_error);
int connect_pipe(int *int_fd, int *ext_fd);
#endif //TESR_COMMON_H
