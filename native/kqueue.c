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

const static int MAX_FD_NUM = 1024 * 128;
const static int BUFFER_SIZE = 1024;

struct kevent changes[MAX_FD_NUM];
struct kevent events[MAX_FD_NUM];
struct kevent all[MAX_FD_NUM];

int change_count = 0;
int event_count = 0;
int all_count = 0;

int ready_fds[MAX_FD_NUM];
int ready_fd_operations[MAX_FD_NUM];

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

int find(struct kevent *list, int size, int fd, int filter){
    for(int i=0; i<size; i++){
        struct kevent *ev = list + i;
        if( (int)ev->ident == fd && ev->filter == filter ){
            return i;
        }
    }

    return -1;
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
    if( flag_id & 1){
        flag = EV_ADD;

        // whether in changes list
        int change_pos = find(changes, change_count, fd, filter);
        // new one
        if( change_pos == -1 ){
            // if added before, skip
            if( find(all, all_count, fd, filter) != -1 ) return;

            // new change
            EV_SET(&changes[change_count++], fd, filter, flag, 0, 0, 0);
            // one more event expected
            event_count++;

            // record this addition
            EV_SET(&all[all_count++], fd, filter, flag, 0, 0, 0);
        }
        // already added once, overriding
        else{
            EV_SET(&changes[change_pos], fd, filter, flag, 0, 0, 0);
        }
    }

    if( flag_id & 2 ){
        flag = EV_DELETE;

        int change_pos = find(changes, change_count, fd, filter);
        // new one
        if( change_pos == -1 ){
            int all_pos;
            // no record, do nothing
            if( (all_pos = find(all, all_count, fd, filter)) == -1 ) return;

            EV_SET(&changes[change_count++], fd, filter, flag, 0, 0, 0);
            // one less event expected
            event_count--;

            // delete the current event in all list
            all[all_pos] = all[all_count-1];
            all_count--;
        }else{
            EV_SET(&changes[change_pos], fd, filter, flag, 0, 0, 0);
        }
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