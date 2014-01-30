#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
#include "tesr_types.h"
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int passes_filters(char *ip, tesr_filter_t *filters);
int is_correctly_formatted(char *buffer);
int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters, rate_limiter_t *rate_limiter);
const char *get_lock_error_string(int lock_error);
int connect_pipe(int *int_fd, int *ext_fd);

queue_data_t *create_queue_data();
int init_queue_data(queue_data_t *thiz);//TODO(nick): should use this
void destroy_queue_data(queue_data_t *thiz);
tesr_filter_t *create_filter();
int init_filter(tesr_filter_t *thiz, const char *filter);
void destroy_filter(tesr_filter_t *thiz);
tesr_filter_t *copy_filters_list(tesr_filter_t *head);
void destroy_filters_list(tesr_filter_t *head);

rate_limit_struct_t *create_rate_limit();
int init_rate_limit(rate_limit_struct_t *thiz, const char *ip);
void destroy_rate_limit(rate_limit_struct_t *thiz);
void destroy_rate_limit_map(rate_limit_struct_t *map);

#endif //TESR_COMMON_H
