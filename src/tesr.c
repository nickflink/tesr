//
// Threaded Echo ServeR
// Author: Nick Flink
//

#include <arpa/inet.h>
#include <ev.h>
#include <pthread.h>
#include <stdio.h> // for puts
#include <stdlib.h>
#include <string.h>
#include <tesr_common.h>
#include <tesr_config.h>
#include <tesr_worker.h>
#include <unistd.h> // for pipe
#include <utlist.h>
//typedef struct worker_data_t {
//    char buffer[BUFFER_LEN];
//    struct sockaddr_in addr;
//    socklen_t bytes;
//    int addr_len;
//    struct worker_data_t *next;
//} worker_data_t;
typedef struct main_thread_t {
    int sd;
    struct sockaddr_in addr;
    struct ev_loop* event_loop;
    struct ev_io udp_read_watcher;
} main_thread_t;
//typedef struct worker_thread_t {
//    int sd;
//    struct sockaddr_in addr;
//    int inbox_fd;
//    int outbox_fd;
//    pthread_t thread;
//    struct ev_loop* event_loop;
//    struct ev_io inbox_watcher;
//    main_thread_t *main_thread;
//    worker_data_t *queue;
//} worker_thread_t;
main_thread_t main_thread;
worker_thread_t *worker_threads;
tesr_config_t tesr_config;

//pthread_mutex_t lock;
int recv_count = 0;

ev_async async_watcher;

//void* worker_thread_start(void* args) {
//    printf("worker_thread_start=%d", (int)pthread_self());
//    int port = 1981;
//    int ret = bind_dgram_socket(&worker_thread.sd, &worker_thread.addr, port);
//    if (ret != 0) {
//        perror("could not bind dgram socket");
//    }
//    ev_loop(worker_thread.event_loop, 0);
//    return NULL;
//}

//static void async_cb (EV_P_ ev_async *w, int revents) {
//    pthread_mutex_lock(&lock);     //Don't forget locking
//    worker_data_t *data, *tmp;
//    LL_FOREACH_SAFE(worker_thread.queue, data, tmp) {
//        sendto(worker_thread.sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
//        //TODO: free data
//        usleep(10);
//        ++async_count;
//        printf("tid = %d async_cb %d\n", (int)pthread_self(), async_count);
//        LL_DELETE(worker_thread.queue, data);
//    }
//    pthread_mutex_unlock(&lock);   //Don't forget unlocking
//}

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    if (ev_async_pending(&worker_threads[0].async_watcher)==0) { //the event has not yet been processed (or even noted) by the event loop? (i.e. Is it serviced? If yes then proceed to)
        ev_async_send(worker_threads[0].event_loop, &worker_threads[0].async_watcher); //Sends/signals/activates the given ev_async watcher, that is, feeds an EV_ASYNC event on the watcher into the event loop.
    }
    worker_data_t *data = ((worker_data_t *)malloc(sizeof(worker_data_t)));
    data->addr_len = sizeof(struct sockaddr_in);
    data->bytes = recvfrom(main_thread.sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
//DOLOCK
    pthread_mutex_lock(&worker_threads[0].lock);     //Don't forget locking
    LL_APPEND(worker_threads[0].queue, data);
    ++recv_count;
    printf("tid = %d udp_read_cb %d\n", (int)pthread_self(), recv_count);
    pthread_mutex_unlock(&worker_threads[0].lock);   //Don't forget unlocking
//UNLOCK
}

int main(int argc, char** argv) {
    printf("main_thread=%d", (int)pthread_self());
    init_config(&tesr_config, argc, argv);
    log_config(&tesr_config);
    int ret = bind_dgram_socket(&main_thread.sd, &main_thread.addr, tesr_config.port);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }
    
    main_thread.event_loop = EV_DEFAULT;  //or ev_default_loop (0);
    //Initialize pthread

    worker_threads = new_workers(1);

    pthread_mutex_init(&worker_threads[0].lock, NULL);
    //pthread_t thread;

    // This loop sits in the pthread
    worker_threads[0].event_loop = ev_loop_new(0);

    //This block is specifically used pre-empting thread (i.e. temporary interruption and suspension of a task, without asking for its cooperation, with the intention to resume that task later.)
    //This takes into account thread safety
    ev_async_init(&worker_threads[0].async_watcher, async_echo_cb);
    ev_async_start(worker_threads[0].event_loop, &worker_threads[0].async_watcher);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&worker_threads[0].thread, &attr, worker_thread_start, &worker_threads[0]);

    ev_io_init(&main_thread.udp_read_watcher, udp_read_cb, main_thread.sd, EV_READ);
    ev_io_start(main_thread.event_loop, &main_thread.udp_read_watcher);

    // now wait for events to arrive
    ev_loop(main_thread.event_loop, 0);
    //Wait on threads for execution
    pthread_join(worker_threads[0].thread, NULL);

    pthread_mutex_destroy(&worker_threads[0].lock);
    return 0;
}
