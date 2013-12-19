#include "tesr_common.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

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
