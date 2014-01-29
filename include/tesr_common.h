#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
#include "tesr_types.h"
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int passes_filters(char *ip, tesr_filter_t *filters);
int is_correctly_formatted(char *buffer);
void work_on_queue(tesr_queue_t *inbox, tesr_queue_t *outbox, tesr_filter_t *filters, rate_limiter_t *rate_limiter);
void log_lock_error(int lock_error);
#endif //TESR_COMMON_H
