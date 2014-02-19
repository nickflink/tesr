#ifndef TESR_RATE_LIMITER_H
#define TESR_RATE_LIMITER_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
#include "tesr_types.h"
rate_limiter_t *create_rate_limiter();
void init_rate_limiter(rate_limiter_t *thiz, int irl_max, int irl_inactivity_timeout, int irl_garbage_collect_count);
void destroy_rate_limiter(rate_limiter_t *thiz);
int prune_expired_ips(rate_limiter_t *thiz);
int is_under_rate_limit(rate_limiter_t *thiz, const char *ip);
#endif //TESR_RATE_LIMITER_H
