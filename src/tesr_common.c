#include "tesr_common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for strtoll
#include "tesr_queue.h"
#include "tesr_types.h"
#include <unistd.h> // for pipe
#include <utlist.h>
#include <uthash.h>

int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port) {
    int ret = 1;
    int sockd;
    struct sockaddr_in inaddr;
    sockd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&inaddr, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockd, (struct sockaddr*) &inaddr, sizeof(inaddr)) != 0) {
        LOG_ERROR("binding to port %d failed\n", port);
        ret = 0;
    }
    *sd = sockd;
    *addr = inaddr;
    return ret;
}

int is_correctly_formatted(char *buffer) {
    int ret = 0;
    char *endptr = NULL;
    int base = 10;
    long long int echo = strtoll(buffer, &endptr, base);
    if(buffer != NULL && buffer[0] != '\0' && endptr != NULL && endptr[0] == '\0' && echo > 0) {
        ret = 1;
    } else {
        if(endptr == NULL) {
            endptr = "NULL";
        }
        LOG_INFO("[KO] stroll(%s, %s, %d) = %llu", buffer, endptr, base, echo);
    }
    return ret;
}

int passes_filters(char *ip, tesr_filter_t *filters) {
    int ret = 1;
    tesr_filter_t *element;
    LL_FOREACH(filters, element) {
        if(element) {
            regex_t regex;
            int reti;
            // Cle regular expression
            reti = regcomp(&regex, element->filter, 0);
            if( reti ) {
                LOG_ERROR("Could not compile regex\n");
            }
            // Ete regular expression
            reti = regexec(&regex, ip, 0, NULL, 0);
            if( !reti ) {
                LOG_DEBUG("[KO] Filtered Out By %s!\n", element->filter);
                ret = 0;
                break;
            } else if( reti == REG_NOMATCH ) {
                LOG_DEBUG("[OK] Passed Filter %s\n", element->filter);
                ret = 1;
            } else {
                char msgbuf[100];
                regerror(reti, &regex, msgbuf, sizeof(msgbuf));
                LOG_ERROR("REGEX match failed: %s\n", msgbuf);
            }
            // Free compiled regular expression if you want to use the regex_t again
            regfree(&regex);
        }
    }
    return ret;
}

int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters, rate_limiter_t *rate_limiter) {
    int ret = 0;
    if(buffer != NULL) {
        buffer[bytes] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(addr->sin_family, &addr->sin_addr, ip, INET_ADDRSTRLEN);
        LOG_DEBUG("should_echo:%s from:%s:%d\n", buffer, ip, ntohs(addr->sin_port));
        if(is_correctly_formatted(buffer) && passes_filters(ip, filters) && is_under_rate_limit(rate_limiter, ip)) {
            ret = 1;
        }
    }
    return ret;
}

const char *get_lock_error_string(int lock_error) {
    switch(lock_error) {
        case EINVAL:
            return "The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling thread's priority is higher than the mutex's current priority ceiling.\n-- OR --\nThe value specified by mutex does not refer to an initialized mutex object.";
        case EBUSY:
            return "The mutex could not be acquired because it was already locked.";
        break;
        case EAGAIN:
            return "The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.";
        break;
        case EDEADLK:
            return "A deadlock condition was detected or the current thread already owns the mutex.";
        break;
        case EPERM:
            return "The current thread does not own the mutex.";
        break;
        case 0:
            return "Success";
        default:
        break;
    }
    return "UNKNOWN lock failure state.";
}

int connect_pipe(int *int_fd, int *ext_fd) {
    int fds[2];
    if(pipe(fds)) {
        LOG_ERROR("Can't create notify pipe");
        return 0;
    }
    *int_fd = fds[0];
    *ext_fd = fds[1];
     return 1;
}

queue_data_t *create_queue_data() {
    queue_data_t *thiz = NULL;
    thiz = (queue_data_t*)malloc(sizeof(queue_data_t));
    TESR_LOG_ALLOC(thiz, queue_data_t);
    return thiz;
}

void destroy_queue_data(queue_data_t *thiz) {
    if(thiz) {
        TESR_LOG_FREE(thiz, queue_data_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free queue_data_t * as it is NULL");
    }
}

tesr_filter_t *create_filter() {
    tesr_filter_t *thiz = NULL;
    thiz = (tesr_filter_t*)malloc(sizeof(tesr_filter_t));
    TESR_LOG_ALLOC(thiz, tesr_filter_t);
    return thiz;
}

int init_filter(tesr_filter_t *thiz, const char *filter) {
    strncpy(thiz->filter, filter, INET_ADDRSTRLEN);
    return 1;
}

void destroy_filter(tesr_filter_t *thiz) {
    if(thiz) {
        TESR_LOG_FREE(thiz, tesr_filter_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free tesr_filter_t * as it is NULL");
    }
}

tesr_filter_t *copy_filters_list(tesr_filter_t *head) {
    tesr_filter_t *copy_head = NULL;
    tesr_filter_t *copy = NULL;
    tesr_filter_t *filter = NULL;
    LL_FOREACH(head, filter) {
        LOG_DEBUG("Prepend> %s\n", filter->filter);
        copy = create_filter();
        init_filter(copy, filter->filter);
        LL_PREPEND(copy_head, copy);
    }
    return copy_head;
}

void destroy_filters_list(tesr_filter_t *head) {
    tesr_filter_t *filter = NULL;
    tesr_filter_t *tmp = NULL;
    // delete each element, use the safe iterator
    LL_FOREACH_SAFE(head,filter,tmp) {
      LL_DELETE(head,filter);
      destroy_filter(filter);
    }
}

rate_limit_struct_t *create_rate_limit() {
    rate_limit_struct_t *thiz = NULL;
    thiz = (rate_limit_struct_t*)malloc(sizeof(rate_limit_struct_t));
    TESR_LOG_ALLOC(thiz, rate_limit_struct_t);
    return thiz;
}

int init_rate_limit(rate_limit_struct_t *thiz, const char *ip) {
    strncpy(thiz->ip, ip, INET_ADDRSTRLEN);
    thiz->count = 0;
    time(&thiz->last_check);
    return 1;
}

void destroy_rate_limit(rate_limit_struct_t *thiz) {
    if(thiz) {
        TESR_LOG_FREE(thiz, rate_limit_struct_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free rate_limit_struct_t * as it is NULL");
    }
}

void destroy_rate_limit_map(rate_limit_struct_t *map) {
    rate_limit_struct_t *rl = NULL;
    rate_limit_struct_t *tmp = NULL;
    //UT_hash_handle hh; //generated by macro
    HASH_ITER(hh, map, rl, tmp) {
        HASH_DEL(map, rl);  // delete; rate_limit_map advances to next
        destroy_rate_limit(rl);
    }
}

