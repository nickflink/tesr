//This program is demo for using pthreads with libev.
//Try using Timeout values as large as 1.0 and as small as 0.000001
//and notice the difference in the output

//(c) 2009 debuguo
//(c) 2013 enthusiasticgeek for stack overflow
//Free to distribute and improve the code. Leave credits intact

#include <arpa/inet.h>
#include <ev.h>
#include <pthread.h>
#include <stdio.h> // for puts
#include <stdlib.h>
//#include <sys/socket.h>
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
    struct ev_loop* event_loop;
    //struct ev_timer timeout_watcher;
    struct ev_io udp_read_watcher;
} main_thread_t;

typedef struct worker_thread_t {
    int sd;
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
//ev_timer timeout_watcher;
int timeout_count = 0;

//ev_async async_watcher;
//int async_count = 0;

static void inbox_cb_w(EV_P_ ev_io *w, int revents);

void* start_worker(void* args) {
    struct worker_thread_t *wt = args;
    if (ev_userdata(wt->event_loop) == NULL) {
        pthread_t self = pthread_self();
        printf("worker_thread=%d\n", (int)self); // Here one could initiate another timeout watcher
        ev_set_userdata(wt->event_loop, (void*)self);
        //
        //Socket
        //
        struct sockaddr_in addr;
        int port = 1998;
        wt->sd = socket(PF_INET, SOCK_DGRAM, 0);
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(wt->sd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
            perror("worker_thread bind failure");
        }
    }
    ev_run(wt->event_loop, EVFLAG_AUTO);
    return NULL;
}

static void inbox_cb_w(EV_P_ ev_io *w, int revents) {
    worker_data_t data;
    //printf("pthread = %d readable\n", (int)pthread_self());
    size_t len = sizeof(struct worker_data_t);
    //if (read(w->fd, &data, len) != len) {
//check the size
//fseek(w->fd, 0L, SEEK_END);
//int sz = ftell(w->fd);
//fseek(w->fd, 0L, SEEK_SET);
struct stat buf;
fstat(w->fd, &buf);
int sz = buf.st_size;
    int ret = read(w->fd, &data, len);
    if (ret != len) {
        perror("Can't read from connection notify pipe\n");
        printf("[KO] ret = %d != %d len sz = %d\n", ret, (int)len, sz);
    } else {
        printf("[OK] ret = %d == %d len sz = %d\n", ret, (int)len, sz);
        static int send_count = 0;
        printf("thread = %d send_count %d\n", (int)pthread_self(), send_count++);
        //printf("pthread = %d readable buffer %s\n", (int)pthread_self(), data.buffer);
        //usleep(100);
        sendto(worker_thread.sd, data.buffer, data.bytes, 0, (struct sockaddr*) &data.addr, sizeof(data.addr));
    }
}

//static void async_cb (EV_P_ ev_async *w, int revents) {
//    //puts ("async ready");
//    pthread_mutex_lock(&lock);     //Don't forget locking
//    ++async_count;
//    printf("pthread = %d async = %d, timeout = %d \n", (int)pthread_self(), async_count, timeout_count);
//    pthread_mutex_unlock(&lock);   //Don't forget unlocking
//}

//static void timeout_cb (EV_P_ ev_timer *w, int revents) // Timer callback function
//{
//    printf("pthread = %d timeout \n", (int)pthread_self());
//    //if (ev_async_pending(&async_watcher)==0) { //the event has not yet been processed (or even noted) by the event loop? (i.e. Is it serviced? If yes then proceed to)
//    //    printf("pthread = %d ev_async_send\n", (int)pthread_self());
//    //    ev_async_send(worker_thread.event_loop, &async_watcher); //Sends/signals/activates the given ev_async watcher, that is, feeds an EV_ASYNC event on the watcher into the event loop.
//    //}
//
//    pthread_mutex_lock(&lock);     //Don't forget locking
//    ++timeout_count;
//    pthread_mutex_unlock(&lock);   //Don't forget unlocking
//    w->repeat = timeout;
//    //Farm out work to worker
//    printf("pthread = %d farm out work\n", (int)pthread_self());
//    worker_data.data = 1337;
//    size_t len = sizeof(struct worker_data_t);
//    if (write(worker_thread.outbox_fd, &worker_data, len) != len) {
//        perror("Fail to writing to connection notify pipe");
//    }
//    //END Farm out work to worker
//    //ev_timer_again(loop, &timeout_watcher); //Start the timer again.
//}

static void udp_read_cb(EV_P_ ev_io *w, int revents) {
    //int addr_len = sizeof(struct sockaddr_in);
    //struct sockaddr_in addr;
    //char buffer[1024];
    worker_data_t *data = ((worker_data_t *)malloc(sizeof(worker_data_t)));
    data->addr_len = sizeof(struct sockaddr_in);
    data->bytes = recvfrom(main_thread.sd, data->buffer, sizeof(data->buffer) - 1, 0, (struct sockaddr*) &data->addr, (socklen_t *) &data->addr_len);
//DOLOCK
    LL_APPEND(worker_thread.queue, data);
    size_t len = sizeof(struct worker_data_t);
    if (write(worker_thread.outbox_fd, data, len) != len) {
        perror("Fail to writing to connection notify pipe");
    }
//UNLOCK
    //inet_ntop(addr.sin_family, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    static int recv_count = 0;
    printf("thread = %d recv_count %d\n", (int)pthread_self(), recv_count++);
    //usleep(100);
    //sendto(main_thread.sd, data->buffer, data->bytes, 0, (struct sockaddr*) &data->addr, sizeof(data->addr));
}

int main (int argc, char** argv)
{
    //if (argc < 2) {
    //    puts("Timeout value missing.\n./demo <timeout>");
    //    return -1;
    //}
    //timeout = atof(argv[1]);
    //struct ev_loop *loop = EV_DEFAULT;  //or ev_default_loop (0);
    main_thread.event_loop = EV_DEFAULT;  //or ev_default_loop (0);
    //
    // Socket setup
    //
    struct sockaddr_in addr;
    int port = 1989;
    main_thread.sd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(main_thread.sd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        perror("main_thread bind failure");
    }
    ev_io_init(&main_thread.udp_read_watcher, udp_read_cb, main_thread.sd, EV_READ);
    for(int i = 0; i < 1; i++) {
        int fds[2];
        if(pipe(fds)) {
            perror("Can't create notify pipe");
            return 0;
        }
        worker_thread.queue = NULL;
        worker_thread.inbox_fd = fds[0];
        worker_thread.outbox_fd = fds[1];
        printf("main_thread=%d\n", (int)pthread_self());
        //Initialize pthread
        pthread_mutex_init(&lock, NULL);
        // This loop sits in the pthread
        worker_thread.event_loop = ev_loop_new(EVFLAG_AUTO);
        worker_thread.main_thread = &main_thread;
        ev_io_init(&worker_thread.inbox_watcher, inbox_cb_w, worker_thread.inbox_fd, EV_READ);
        ev_io_start(worker_thread.event_loop, &worker_thread.inbox_watcher);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&worker_thread.thread, &attr, start_worker, &worker_thread);
    }
    //This block is specifically used pre-empting thread (i.e. temporary interruption and suspension of a task, without asking for its cooperation, with the intention to resume that task later.)
    //This takes into account thread safety
    //pthread_t thread;
    //ev_async_init(&async_watcher, async_cb);
    //ev_async_start(worker_thread.event_loop, &async_watcher);
    //s = pthread_create(&tinfo[tnum].thread_id, &attr, &thread_start, &tinfo[tnum]);

    //ev_timer_init(&main_thread.timeout_watcher, timeout_cb, timeout, 0.); // Non repeating timer. The timer starts repeating in the timeout callback function
    //ev_timer_start (main_thread.event_loop, &main_thread.timeout_watcher);
    ev_io_start(main_thread.event_loop, &main_thread.udp_read_watcher);

    // now wait for events to arrive
    ev_loop(main_thread.event_loop, 0);
    //Wait on threads for execution
    pthread_join(worker_thread.thread, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
