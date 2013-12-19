#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int should_echo(char *buffer, socklen_t bytes);
#endif //TESR_COMMON_H
