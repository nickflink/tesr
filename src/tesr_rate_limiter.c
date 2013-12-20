#include "tesr_rate_limiter.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for strtoll
#include <tesr_common.h>
#include <utlist.h>
#include <uthash.h>

rate_limiter_t *create_rate_limiter() {
    rate_limiter_t *rate_limiter = NULL;
    rate_limiter = (rate_limiter_t*)malloc(sizeof(rate_limiter_t));
    return rate_limiter;
}

void init_rate_limiter(rate_limiter_t *rate_limiter, int ip_rate_limit_max, int ip_rate_limit_period) {
    rate_limiter->rate_limit_map = NULL;
    pthread_mutex_init(&rate_limiter->lock, NULL);
    rate_limiter->ip_rate_limit_max = ip_rate_limit_max;
    rate_limiter->ip_rate_limit_period = ip_rate_limit_period;
}

void destroy_rate_limiter(rate_limiter_t *rate_limiter) {
    if(rate_limiter) {
        free(rate_limiter);
    } else {
        LOG_ERROR("can not free rate_limiter it is NULL");
    }
}

int is_under_rate_limit(rate_limiter_t *rate_limiter, char *ip) {
    int ret = 1;
    pthread_mutex_lock(&rate_limiter->lock);     //Don't forget locking
    //ADD RATE LIMIT
    rate_limit_struct_t *rl = NULL;
    HASH_FIND_STR( rate_limiter->rate_limit_map, ip, rl );
    if(!rl) {
        rl = malloc(sizeof(rate_limit_struct_t));
        time(&rl->last_check);
        printf("Current local time and date: %s", ctime(&rl->last_check));
        strncpy(rl->ip, ip, INET_ADDRSTRLEN);
        rl->count = 1;
        HASH_ADD_STR( rate_limiter->rate_limit_map, ip, rl);
    }
    ++rl->count;
    if(rl->count > rate_limiter->ip_rate_limit_max) {
        LOG_DEBUG("[OK] rl->count=%d > %d=rate_limiter->ip_rate_limit_max\n", rl->count, rate_limiter->ip_rate_limit_max);
        ret = 0;
    } else {
        LOG_DEBUG("[KO] rl->count=%d <= %d=rate_limiter->ip_rate_limit_max\n", rl->count, rate_limiter->ip_rate_limit_max);
    }
    //EXPIRE RATE LIMIT
    rate_limit_struct_t *tmp = NULL;
    time_t now;
    time(&now);
    double time_elapsed = 0.0;
    //UT_hash_handle hh;
    HASH_ITER(hh, rate_limiter->rate_limit_map, rl, tmp) {
        time_elapsed = difftime(now,rl->last_check);
        LOG_DEBUG("rl{%s} = %d [%f]sec elapsed\n", rl->ip, rl->count, time_elapsed);
        if(time_elapsed > rate_limiter->ip_rate_limit_period) {
            HASH_DEL(rate_limiter->rate_limit_map, rl);  /* delete; users advances to next */
            free(rl);            /* optional- if you want to free  */
        }
    }
    pthread_mutex_unlock(&rate_limiter->lock);     //Don't forget locking
    return ret;
}

