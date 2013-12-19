#ifndef TESR_COMMON_H
#define TESR_COMMON_H
struct sockaddr_in;
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
#endif //TESR_COMMON_H
