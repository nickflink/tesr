#ifndef TESR_RATE_LIMITER_H
#define TESR_RATE_LIMITER_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#include <uthash.h>
typedef struct rate_limit_struct_t {
    char ip[INET_ADDRSTRLEN]; /* we'll use this field as the key */
    time_t last_check;
    int count;
    UT_hash_handle hh;        /* makes this structure hashable */
} rate_limit_struct_t;

typedef struct rate_limiter_t {
    rate_limit_struct_t *rate_limit_map;
    pthread_mutex_t lock;
    int ip_rate_limit_max;
    int ip_rate_limit_period;
} rate_limiter_t;

rate_limiter_t *create_rate_limiter();
void init_rate_limiter(rate_limiter_t *rate_limiter, int ip_rate_limit_max, int ip_rate_limit_period);
void destroy_rate_limiter(rate_limiter_t *rate_limiter);
int is_under_rate_limit(rate_limiter_t *rate_limiter, char *ip);
#endif //TESR_RATE_LIMITER_H
