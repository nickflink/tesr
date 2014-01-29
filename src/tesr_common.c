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
#include <utlist.h>

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

static int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters, rate_limiter_t *rate_limiter) {
    int ret = 0;
    if(buffer != NULL) {
        buffer[bytes] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(addr->sin_family, &addr->sin_addr, ip, INET_ADDRSTRLEN);
        LOG_DEBUG("should_echo:%s from:%s:%d\n", buffer, ip, ntohs(addr->sin_port));
        if(is_correctly_formatted(buffer)) {
            if(passes_filters(ip, filters)) {
                if(is_under_rate_limit(rate_limiter, ip)) {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}

//void process_queue_data(queue_data_t *data, main_thread_t *main_thread, tesr_filter_t *filters, rate_limiter_t *rate_limiter, int th) {
void work_on_queue(tesr_queue_t *inbox, tesr_queue_t *outbox, tesr_filter_t *filters, rate_limiter_t *rate_limiter) {
    queue_data_t *data = tesr_dequeue(inbox);
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
    if(data) {
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
        if(should_echo(data->buffer, data->bytes, &data->addr, filters, rate_limiter)) {
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
            LOG_DEBUG("[OK]>thread = 0x%zx should_echo\n", (size_t)pthread_self());
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
            tesr_enqueue(outbox, data);
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
        } else {
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
            LOG_DEBUG("[KO]Xthread = 0x%zx should_NOT_echo\n", (size_t)pthread_self());
LOG_DEBUG("LOC:%d::%s::%s thread = 0x%zx\n", __LINE__, __FILE__, __FUNCTION__, (size_t)pthread_self());
        }
    }
}

void log_lock_error(int lock_error) {
    switch(lock_error) {
        case EINVAL:
            LOG_DEBUG("The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling thread's priority is higher than the mutex's current priority ceiling.\n");
            LOG_DEBUG("-- OR --\n");
            LOG_DEBUG("The value specified by mutex does not refer to an initialized mutex object.\n");
        case EBUSY:
            LOG_DEBUG("The mutex could not be acquired because it was already locked.\n");
        break;
        case EAGAIN:
            LOG_DEBUG("The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.");
        break;
        case EDEADLK:
            LOG_DEBUG("A deadlock condition was detected or the current thread already owns the mutex.");
        break;
        case EPERM:
            LOG_DEBUG("The current thread does not own the mutex.");
        break;
        default:
            LOG_DEBUG("UNKNOWN lock failure state.");
        break;
    }
}
