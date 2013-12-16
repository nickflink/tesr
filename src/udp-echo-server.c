// Source: http://www.mail-archive.com/libev@lists.schmorp.de/msg00987.html

#include <arpa/inet.h>
#include <errno.h>
#include <ev.h>
#include <libconfig.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "uthash.h"

//#define DEFAULT_PORT    3333
#define DEFAULT_PORT    7
#define BUF_SIZE        4096

// Lots of globals, what's the best way to get rid of these?
int sd; // socket descriptor
struct sockaddr_in addr;
int addr_len = sizeof(addr);
char ip[INET_ADDRSTRLEN];
char buffer[BUF_SIZE];
char debugCount[BUF_SIZE];
int count = 0;


struct rate_limit_struct {
    char ip[INET_ADDRSTRLEN]; /* we'll use this field as the key */
    int count;
    UT_hash_handle hh;        /* makes this structure hashable */
};

struct rate_limit_struct *rate_limit_map = NULL;

// This callback is called when data is readable on the UDP socket.
static void udp_cb(EV_P_ ev_io *w, int revents) {
    //puts("udp socket has become readable for the %d time(s)", ++count);
    snprintf(debugCount, BUF_SIZE, "udp socket has become readable for the %d time(s)", ++count);
    puts(debugCount);
    socklen_t bytes = recvfrom(sd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*) &addr, (socklen_t *) &addr_len);
    inet_ntop(addr.sin_family, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    //inet_pton(AF_INET, ip)

    // add a null to terminate the input, as we're going to use it as a string
    buffer[bytes] = '\0';
    char *endptr = NULL;
    int base = 10;
    long long int echo = strtoll(buffer, &endptr, base);
    if(buffer != NULL && buffer[0] != '\0' && endptr != NULL && endptr[0] == '\0') {
        // Echo it back.
        // WARNING: this is probably not the right way to do it with libev.
        // Question: should we be setting a callback on sd becomming writable here instead?
        struct rate_limit_struct *rl = NULL;
        HASH_FIND_STR( rate_limit_map, ip, rl );
        if(!rl) {
            rl = malloc(sizeof(struct rate_limit_struct));
            strncpy(rl->ip, ip, INET_ADDRSTRLEN);
            rl->count = 1;
            HASH_ADD_STR( rate_limit_map, ip, rl);
        }
        ++rl->count;
        printf("[OK] udp client said: %s from ip: %s:%d\n", buffer, ip, ntohs(addr.sin_port));
        sendto(sd, buffer, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));
    } else {
        if(endptr == NULL) {
            endptr = "NULL";
        }
        printf("[KO] stroll(%s, %s, %d) = %llu", buffer, endptr, base, echo);
    }
}

static void idle_cb(struct ev_loop *loop, struct ev_idle *w, int revents) {
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

int main(void) {
    int port = DEFAULT_PORT;
    puts("udp_echo server started...");

    // Setup a udp listening socket.
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0)
        perror("bind");

    // Do the libev stuff.
    struct ev_loop *loop = ev_default_loop(0);
    ev_io udp_watcher;
    ev_idle idle_watcher;
    ev_io_init(&udp_watcher, udp_cb, sd, EV_READ);
    ev_io_start(loop, &udp_watcher);
    ev_idle_init(&idle_watcher, idle_cb);
    ev_idle_start(loop, &idle_watcher);
    ev_loop(loop, 0);

    // This point is never reached.
    close(sd);
    return EXIT_SUCCESS;
}
