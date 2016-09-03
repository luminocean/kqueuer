#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * This is a wrapper file of kqueue related functions for easier uses
 */

const static int KQ_READ = 1;
const static int KQ_WRITE = 2;

const static int MAX_FD_NUM = 1024;
const static int BUFFER_SIZE = 1024;

struct kevent changes[MAX_FD_NUM];
struct kevent events[MAX_FD_NUM];

int ready_fds[MAX_FD_NUM];
int ready_fd_operations[MAX_FD_NUM];

int change_count = 0;
int event_count = 0;

int kq; // kqueue object

// quit current process with an error message
int quit(const char *msg){
    perror(msg);
    exit(1);
}

// initialize kqueue environment
// this function must be called before any other subsequent calls
void initialize(){
    if( (kq = kqueue()) == -1 ) quit("kqueue error");
    for(int i=0; i<MAX_FD_NUM; i++){
        ready_fds[i] = -1;
        ready_fd_operations[i] = -1;
    }
}

int ready_fd(int i){
    return ready_fds[i];
}

int fd_operation(int i){
    return ready_fd_operations[i];
}

void ev_set(int fd, int filter_id, int flag_id){
    // resolve filter
    int filter = 0;
    if( filter_id == 1 )
        filter = EVFILT_READ;
    else if( filter_id == 2 )
        filter = EVFILT_WRITE;
    else{
        quit("Invalid filter");
    }

    int flag = 0;
    if( flag_id & 1 ){
        flag = EV_ADD;
        EV_SET(&changes[change_count++], fd, filter, flag, 0, 0, 0);
        // one more event expected
        event_count++;
    }

    if( flag_id & 2 ){
        flag = EV_DELETE;
        EV_SET(&changes[change_count++], fd, filter, flag, 0, 0, 0);
        // one less event expected
        event_count--;
    }
}

int wait(){
    int nev = kevent(kq, changes, change_count, events, event_count, NULL);
    change_count = 0; // ready to accept new changes

    // set readied events
    for(int i=0; i<nev; i++){
        struct kevent event = events[i];
        if( event.flags & EV_ERROR ){
            quit(strerror(event.data));
        }

        int operation = 0;
        if(event.filter == EVFILT_READ)
            operation = 1;
        else if(event.filter == EVFILT_WRITE)
            operation = 2;

        ready_fds[i] = (int)event.ident;
        ready_fd_operations[i] = operation;
    }

    for(int i=nev; i<nev; i++){
        ready_fds[i] = -1;
        ready_fd_operations[i] = -1;
    }
    return nev;
}