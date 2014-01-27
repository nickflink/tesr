#include "tesr_common.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for strtoll
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

