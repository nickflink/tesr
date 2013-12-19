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
        perror("binding failed");
    }
    *sd = sockd;
    *addr = inaddr;
    return ret;
}

int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters) {
    //
    //FILTER CHECK
    //
    buffer[bytes] = '\0';
    int passed_filter = 1;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(addr->sin_family, &addr->sin_addr, ip, INET_ADDRSTRLEN);
    printf("should_echo:from: %s:%d\n", ip, ntohs(addr->sin_port));
    tesr_filter_t *element;
    LL_FOREACH(filters, element) {
        if(element) {
            printf("filter element %s\n", element->filter);
            regex_t regex;
            int reti;
            // Cle regular expression
            reti = regcomp(&regex, element->filter, 0);
            if( reti ) {
                fprintf(stderr, "Could not compile regex\n"); exit(1);
            }
            // Ete regular expression
            reti = regexec(&regex, ip, 0, NULL, 0);
            if( !reti ) {
                puts("[KO] Filtered Out!");
                passed_filter = 0;
                break;
            } else if( reti == REG_NOMATCH ) {
                puts("[OK] Passed Filter");
            } else {
                char msgbuf[100];
                regerror(reti, &regex, msgbuf, sizeof(msgbuf));
                fprintf(stderr, "REGEX match failed: %s\n", msgbuf);
                //exit(1);
            }
            // Free compiled regular expression if you want to use the regex_t again
            regfree(&regex);
        }
    }
    // add a null to terminate the input, as we're going to use it as a string
    char *endptr = NULL;
    int base = 10;
    long long int echo = strtoll(buffer, &endptr, base);
    if(passed_filter && buffer != NULL && buffer[0] != '\0' && endptr != NULL && endptr[0] == '\0') {
        passed_filter = 0;
        // Echo it back.
        // WARNING: this is probably not the right way to do it with libev.
        // Question: should we be setting a callback on sd becomming writable here instead?
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
        printf("[OK] udp client said: %s \n", buffer);
        //printf("[OK] udp client said: %s from ip: %s:%d\n", buffer, ip, ntohs(addr.sin_port));
        passed_filter = 1;
    } else {
        if(endptr == NULL) {
            endptr = "NULL";
        }
        printf("[KO] stroll(%s, %s, %d) = %llu", buffer, endptr, base, echo);
        passed_filter = 0;
    }
    return passed_filter;
}
