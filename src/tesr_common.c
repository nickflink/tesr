#include "tesr_common.h"

#include <arpa/inet.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for strtoll
#include <utlist.h>

int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port) {
    int ret = 0;
    int sockd;
    struct sockaddr_in inaddr;
    sockd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&inaddr, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockd, (struct sockaddr*) &inaddr, sizeof(inaddr)) != 0) {
        LOG_ERROR("binding failed");
    }
    *sd = sockd;
    *addr = inaddr;
    return ret;
}

static int is_correctly_formatted(char *buffer) {
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

static int passes_filters(char *ip, tesr_filter_t *filters) {
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
                LOG_DEBUG("[KO] Filtered Out By %s!", element->filter);
                ret = 0;
                break;
            } else if( reti == REG_NOMATCH ) {
                LOG_DEBUG("[OK] Passed Filter %s", element->filter);
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

int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters) {
    int ret = 0;
    if(buffer != NULL) {
        buffer[bytes] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(addr->sin_family, &addr->sin_addr, ip, INET_ADDRSTRLEN);
        LOG_DEBUG("should_echo:%s from:%s:%d\n", buffer, ip, ntohs(addr->sin_port));
        if(is_correctly_formatted(buffer)) {
            if(passes_filters(ip, filters)) {
                ret = 1;
            }
        }
    }
    return ret;
#if RATE_LIMIT
        struct rate_limit_struct *rl = NULL;
        HASH_FIND_STR( rate_limit_map, ip, rl );
        if(!rl) {
            rl = malloc(sizeof(struct rate_limit_struct));
            strncpy(rl->ip, ip, INET_ADDRSTRLEN);
            rl->count = 1;
            HASH_ADD_STR( rate_limit_map, ip, rl);
        }
        ++rl->count;
#endif //RATE_LIMIT
}
