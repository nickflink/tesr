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
typedef struct main_thread_t {
    int sd;
    struct sockaddr_in addr;
    struct ev_loop* event_loop;
    struct ev_io udp_read_watcher;
} main_thread_t;
main_thread_t main_thread;
int next_thread_idx = 0;
worker_thread_t *worker_threads;
tesr_config_t tesr_config;

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    int th = next_thread_idx;
    printf("waking up th=%d\n", th);
    if(++next_thread_idx >= tesr_config.num_worker_threads) {
        next_thread_idx = 0;
    }
#ifndef USE_PIPES
    if (ev_async_pending(&worker_threads[th].async_watcher)==0) { //the event has not yet been processed (or even noted) by the event loop? (i.e. Is it serviced? If yes then proceed to)
        ev_async_send(worker_threads[th].event_loop, &worker_threads[th].async_watcher); //Sends/signals/activates the given ev_async watcher, that is, feeds an EV_ASYNC event on the watcher into the event loop.
    }
#endif //USE_PIPES
    worker_data_t *data = ((worker_data_t *)malloc(sizeof(worker_data_t)));
    data->addr_len = sizeof(struct sockaddr_in);
    data->bytes = recvfrom(main_thread.sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
//DOLOCK
    pthread_mutex_lock(&worker_threads[th].lock);     //Don't forget locking
    LL_APPEND(worker_threads[th].queue, data);
    static int recv_count = 0;
    ++recv_count;
    printf("tid = %d udp_read_cb %d\n", (int)pthread_self(), recv_count);
#ifdef USE_PIPES
    size_t len = sizeof(th);
    if (write(worker_threads[th].outbox_fd, &th, len) != len) {
        perror("Fail to writing to connection notify pipe");
    }
#endif
    pthread_mutex_unlock(&worker_threads[th].lock);   //Don't forget unlocking
//UNLOCK
}

int main(int argc, char** argv) {
    printf("main_thread=%d", (int)pthread_self());
    init_config(&tesr_config, argc, argv);
    log_config(&tesr_config);
    if(tesr_config.num_worker_threads == 0) {
        perror("at least one worker is required");
        return 0;
    }
    int ret = bind_dgram_socket(&main_thread.sd, &main_thread.addr, tesr_config.recv_port);
    if (ret != 0) {
        perror("could not bind dgram socket");
    }
    
    main_thread.event_loop = EV_DEFAULT;  //or ev_default_loop (0);
    //Initialize pthread
    worker_threads = create_workers(tesr_config.num_worker_threads);
    //for(int th = 0; th < tesr_config.num_worker_threads; th++)
    int th = 0;
    tesr_send_port_t *send_port; 
    LL_FOREACH(tesr_config.send_ports, send_port) {
        init_worker(&worker_threads[th], send_port->port, th);
        //pthread_mutex_init(&worker_threads[th].lock, NULL);
        //// This loop sits in the pthread
        //worker_threads[th].event_loop = ev_loop_new(0);
        //worker_threads[th].port = 1980+th;

        //This block is specifically used pre-empting thread (i.e. temporary interruption and suspension of a task, without asking for its cooperation, with the intention to resume that task later.)
        //This takes into account thread safety
        //ev_async_init(&worker_threads[th].async_watcher, async_echo_cb);
        //ev_async_start(worker_threads[th].event_loop, &worker_threads[th].async_watcher);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&worker_threads[th].thread, &attr, worker_thread_start, &worker_threads[th]);
        ++th;
    }

    ev_io_init(&main_thread.udp_read_watcher, udp_read_cb, main_thread.sd, EV_READ);
    ev_io_start(main_thread.event_loop, &main_thread.udp_read_watcher);

    // now wait for events to arrive
    ev_loop(main_thread.event_loop, 0);
    //Wait on threads for execution
    for(int th = 0; th < tesr_config.num_worker_threads; th++) {
        pthread_join(worker_threads[th].thread, NULL);
        pthread_mutex_destroy(&worker_threads[th].lock);
    }
    return 0;
}
