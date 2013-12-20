#ifndef TESR_COMMON_H
#define TESR_COMMON_H
#include <arpa/inet.h> //needed for sockaddr_in & socklen_t
#define SEP1 "::"$
#define SEP2 "##"$
#define SEP3 ": "$
#define LOG_LOCATION(file, line, function) do{printf("LOC:%d::%s::%s\n",line,file,function);}while(0)
//#define LOG_LOC LOG_LOCATION(__FILE__, __LINE__, __FUNCTION__)
#define LOG_LOC do{}while(0);
#define LOG_DEBUG(format,...) do{}while(0)
//#define LOG_DEBUG(format,...) do{printf("DEBUG:");printf(format,##__VA_ARGS__);}while(0)
#define LOG_INFO(format,...) do{printf("INFO:"format,##__VA_ARGS__);}while(0)
#define LOG_ERROR(format,...) do{fprintf(stderr,"ERROR:"format,##__VA_ARGS__);}while(0)

typedef struct tesr_filter_t {
    char filter[INET_ADDRSTRLEN];
    struct tesr_filter_t *next; /* needed for singly- or doubly-linked lists */
} tesr_filter_t;
int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port);
int should_echo(char *buffer, socklen_t bytes, struct sockaddr_in *addr, tesr_filter_t *filters);
#endif //TESR_COMMON_H
