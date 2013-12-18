// Source: http://www.mail-archive.com/libev@lists.schmorp.de/msg00987.html

#include <arpa/inet.h>
#include <errno.h>
#include <ev.h>
#include <getopt.h>
#include <libconfig.h>
#include <regex.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uthash.h>
#include <utlist.h>
#include <ues-config.h>

//#define DEFAULT_PORT    3333
//#define DEFAULT_PORT    7
#define BUF_SIZE        4096
#define RATE_LIMIT      0

ues_config_t ues_config;
// Lots of globals, what's the best way to get rid of these?
int sd; // socket descriptor
struct sockaddr_in addr;
int addr_len = sizeof(addr);
char ip[INET_ADDRSTRLEN];
char buffer[BUF_SIZE];
//char debugCount[BUF_SIZE];
int count = 0;





#if RATE_LIMIT
struct rate_limit_struct {
    char ip[INET_ADDRSTRLEN]; /* we'll use this field as the key */
    int count;
    UT_hash_handle hh;        /* makes this structure hashable */
};
#endif //RATE_LIMIT

struct rate_limit_struct *rate_limit_map = NULL;

// This callback is called when data is readable on the UDP socket.
static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    //puts("udp socket has become readable for the %d time(s)", ++count);
    //snprintf(debugCount, BUF_SIZE, "udp socket has become readable for the %d time(s)", ++count);
    //puts(debugCount);
    socklen_t bytes = recvfrom(sd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &addr, (socklen_t *) &addr_len);
    inet_ntop(addr.sin_family, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    //
    //FILTER CHECK
    //
    int passed_filter = 1;
    ues_filter_t *element;
    LL_FOREACH(ues_config.filters, element) {
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
    //inet_pton(AF_INET, ip)

    // add a null to terminate the input, as we're going to use it as a string
    buffer[bytes] = '\0';
    char *endptr = NULL;
    int base = 10;
    long long int echo = strtoll(buffer, &endptr, base);
    if(passed_filter && buffer != NULL && buffer[0] != '\0' && endptr != NULL && endptr[0] == '\0') {
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
        printf("[OK] udp client said: %s from ip: %s:%d\n", buffer, ip, ntohs(addr.sin_port));
        sendto(sd, buffer, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));
    } else {
        if(endptr == NULL) {
            endptr = "NULL";
        }
        printf("[KO] stroll(%s, %s, %d) = %llu", buffer, endptr, base, echo);
    }
}

#if RATE_LIMIT
static void periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents) {
    /* iterate over hash elements  */
    if(rate_limit_map) {
      struct rate_limit_struct *rl = NULL, *tmp = NULL;
      //UT_hash_handle hh;
      HASH_ITER(hh, rate_limit_map, rl, tmp) {
          printf("$items{%s} = %d\n", rl->ip, rl->count);
      }
    } else {
      printf(".");
    }
}
#endif //RATE_LIMIT

int main(int argc, char **argv) {
    //char *host_ip = "127.0.0.1";
    //char *netmask = "127.0.0.0";
    //struct in_addr host, mask, broadcast;
    //char broadcast_address[INET_ADDRSTRLEN];
    //if (inet_pton(AF_INET, host_ip, &host) == 1 &&
    //    inet_pton(AF_INET, ne5tmask, &mask) == 1)
    //    broadcast.s_addr = host.s_addr | ~mask.s_addr;
    //else {
    //    fprintf(stderr, "Failed converting strings to numbers\n");
    //    return 1;
    //}
    //if (inet_ntop(AF_INET, &broadcast, broadcast_address, INET_ADDRSTRLEN) != NULL)
    //    printf("Broadcast address of %s with netmask %s is %s\n",
    //        host_ip, netmask, broadcast_address);
    //else {
    //    fprintf(stderr, "Failed converting number to string\n");
    //    return 1;
    //}
    //return 0;

    //
    // REGEX
    //
    char *host_ip = "127.0.0.1";
    char *netmask = "127.0.0.*";
    regex_t regex;
    int reti;
    // Cle regular expression
    reti = regcomp(&regex, netmask, 0);
    if( reti ) {
        fprintf(stderr, "Could not compile regex\n"); exit(1);
    }
    // Ete regular expression
    reti = regexec(&regex, host_ip, 0, NULL, 0);
    if( !reti ) {
        puts("REGEX Match");
    } else if( reti == REG_NOMATCH ) {
        puts("REGEX No match");
    } else {
        char msgbuf[100];
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "REGEX match failed: %s\n", msgbuf);
        exit(1);
    }
    // Free compiled regular expression if you want to use the regex_t again
    regfree(&regex);

    init_config(&ues_config, argc, argv);
    log_config(&ues_config);

    printf("udp-echo-server ver started listening on port %d...\n", ues_config.port);
    //
    // Socket setup
    //
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ues_config.port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        perror("bind");
    }
    //
    // Event Loop and Socket setup
    //
    struct ev_loop *loop = ev_default_loop(0);
    ev_io udp_watcher;
    ev_io_init(&udp_watcher, udp_read_cb, sd, EV_READ);
    ev_io_start(loop, &udp_watcher);
#if RATE_LIMIT
    ev_periodic periodic_watcher;
    ev_periodic_init(&periodic_watcher, periodic_cb, 0., 0.01, 0);
    //Rate Limit over 10 minutes
    ev_periodic_start(loop, &periodic_watcher);
#endif //RATE_LIMIT
    ev_loop(loop, 0);
    // This point is never reached.
    close(sd);
    return EXIT_SUCCESS;
}
