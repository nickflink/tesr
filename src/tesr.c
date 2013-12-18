//This program is demo for using pthreads with libev.
//Try using Timeout values as large as 1.0 and as small as 0.000001
//and notice the difference in the output

//(c) 2009 debuguo
//(c) 2013 enthusiasticgeek for stack overflow
//Free to distribute and improve the code. Leave credits intact

//Threaded Echo ServeR (TESR)

#include <arpa/inet.h>
#include <ev.h>
#include <pthread.h>
#include <stdio.h> // for puts
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for pipe
#include <utlist.h>
#define BUFFER_LEN 1024
typedef struct worker_data_t {
    char buffer[BUFFER_LEN];
    struct sockaddr_in addr;
    socklen_t bytes;
    int addr_len;
    struct worker_data_t *next;
} worker_data_t;
typedef struct main_thread_t {
    int sd;
    struct sockaddr_in addr;
    struct ev_loop* event_loop;
    //struct ev_timer timeout_watcher;
    struct ev_io udp_read_watcher;
} main_thread_t;
typedef struct worker_thread_t {
    int sd;
    struct sockaddr_in addr;
    int inbox_fd;
    int outbox_fd;
    pthread_t thread;
    struct ev_loop* event_loop;
    struct ev_io inbox_watcher;
    main_thread_t *main_thread;
    worker_data_t *queue;
} worker_thread_t;
main_thread_t main_thread;
worker_thread_t worker_thread;
pthread_mutex_t lock;
double timeout = 0.00001;
ev_timer timeout_watcher;
int recv_count = 0;

ev_async async_watcher;
int async_count = 0;

struct ev_loop* loop2;

int bind_dgram_socket(int *sd, struct sockaddr_in *addr, int port) {
    //*sd = socket(PF_INET, SOCK_DGRAM, 0);
    //bzero(addr, sizeof(*addr));
    //addr->sin_family = AF_INET;
    //addr->sin_port = htons(port);
    //addr->sin_addr.s_addr = INADDR_ANY;
    //int ret = bind(*sd, (struct sockaddr*) addr, sizeof(addr));
    //if (ret != 0) {
    //    perror("bind failed");
    //}


    //
    // Socket setup
    //
    int ret = 0;
    int sockd;
    struct sockaddr_in inaddr;
    sockd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&inaddr, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockd, (struct sockaddr*) &inaddr, sizeof(inaddr)) != 0) {
        perror("bind");
    }
    *sd = sockd;
    *addr = inaddr;
    return ret;
}

void* worker_thread_start(void* args)
{
    printf("Inside loop 2"); // Here one could initiate another timeout watcher
    int port = 1981;
    int ret = bind_dgram_socket(&worker_thread.sd, &worker_thread.addr, port);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }



    ev_loop(loop2, 0);       // similar to the main loop - call it say timeout_cb1
    return NULL;
}

static void async_cb (EV_P_ ev_async *w, int revents)
{
    //puts ("async ready");
    //printf("> tid = %d async_cb = %d, timeout = %d\n", (int)pthread_self(), async_count, recv_count);
    pthread_mutex_lock(&lock);     //Don't forget locking
    worker_data_t *data, *tmp;
    LL_FOREACH_SAFE(worker_thread.queue, data, tmp) {
        sendto(worker_thread.sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
        //TODO: free data
        usleep(10);
        ++async_count;
        printf("tid = %d async_cb %d\n", (int)pthread_self(), async_count);
        LL_DELETE(worker_thread.queue, data);
    }
    pthread_mutex_unlock(&lock);   //Don't forget unlocking
    //printf("< tid = %d async_cb = %d, timeout = %d\n", (int)pthread_self(), async_count, recv_count);
}

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    if (ev_async_pending(&async_watcher)==0) { //the event has not yet been processed (or even noted) by the event loop? (i.e. Is it serviced? If yes then proceed to)
        ev_async_send(loop2, &async_watcher); //Sends/signals/activates the given ev_async watcher, that is, feeds an EV_ASYNC event on the watcher into the event loop.
    }
    worker_data_t *data = ((worker_data_t *)malloc(sizeof(worker_data_t)));
    data->addr_len = sizeof(struct sockaddr_in);
    data->bytes = recvfrom(main_thread.sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
    //printf("tid = %d udp_read_cb %d\n", (int)pthread_self(), __LINE__);
//DOLOCK
    pthread_mutex_lock(&lock);     //Don't forget locking
    LL_APPEND(worker_thread.queue, data);
    ++recv_count;
    printf("tid = %d udp_read_cb %d\n", (int)pthread_self(), recv_count);
    pthread_mutex_unlock(&lock);   //Don't forget unlocking
//UNLOCK
    //w->repeat = timeout;
    //ev_timer_again(loop, &timeout_watcher); //Start the timer again.
    //printf("tid = %d udp_read_cb %d\n", (int)pthread_self(), __LINE__);
}

int main (int argc, char** argv)
{
    puts("main");
    printf("%s::%d\n", __FILE__, __LINE__);
    timeout = 1;
    int port = 1989;
    int ret = bind_dgram_socket(&main_thread.sd, &main_thread.addr, port);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }
    
    main_thread.event_loop = EV_DEFAULT;  //or ev_default_loop (0);
    //Initialize pthread
    pthread_mutex_init(&lock, NULL);
    pthread_t thread;

    // This loop sits in the pthread
    loop2 = ev_loop_new(0);

    //This block is specifically used pre-empting thread (i.e. temporary interruption and suspension of a task, without asking for its cooperation, with the intention to resume that task later.)
    //This takes into account thread safety
    ev_async_init(&async_watcher, async_cb);
    ev_async_start(loop2, &async_watcher);
    pthread_create(&thread, NULL, worker_thread_start, NULL);

    //ev_timer_init (&timeout_watcher, timeout_cb, timeout, 0.); // Non repeating timer. The timer starts repeating in the timeout callback function
    ev_io_init(&main_thread.udp_read_watcher, udp_read_cb, main_thread.sd, EV_READ);
    ev_io_start(main_thread.event_loop, &main_thread.udp_read_watcher);

    // now wait for events to arrive
    ev_loop(main_thread.event_loop, 0);
    //Wait on threads for execution
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
