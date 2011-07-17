#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#ifndef _EVBASE_H
#define _EVBASE_H
#ifdef __cplusplus
extern "C" {
#endif
#define E_READ		0x01
#define E_WRITE		0x02
#define E_CLOSE		0x04
#define E_PERSIST	0x08
#define EV_MAX_FD	1024
/*event operating */
#define EOP_PORT        0x00
#define EOP_SELECT      0x01
#define EOP_POLL        0x02
#define EOP_RTSIG       0x03
#define EOP_EPOLL       0x04
#define EOP_KQUEUE      0x05
#define EOP_DEVPOLL     0x06
#define EOP_WIN32       0x07
#define EOP_LIMIT       8
struct _EVENT;
#ifndef _TYPEDEF_EVBASE
#define _TYPEDEF_EVBASE
typedef struct _EVBASE
{
	int efd;
    int maxfd;
	int allowed;
    int evopid;

	void *ev_read_fds;
	void *ev_write_fds;
	void *ev_fds;
	void *evs;
    struct _EVENT **evlist;

	int	    (*init)(struct _EVBASE *);
	int	    (*add)(struct _EVBASE *, struct _EVENT*);
	int 	(*update)(struct _EVBASE *, struct _EVENT*);
	int 	(*del)(struct _EVBASE *, struct _EVENT*);
	int	    (*loop)(struct _EVBASE *, int , struct timeval *tv);
	void	(*reset)(struct _EVBASE *);
	void 	(*clean)(struct _EVBASE *);
    int     (*set_evops)(struct _EVBASE *, int evopid);
}EVBASE;
EVBASE *evbase_init();
#define SET_MAX_FD(evbase, event)                                       \
do{                                                                      \
    if(event->ev_fd > evbase->maxfd) evbase->maxfd = event->ev_fd;      \
}while(0)
#define RESET_MAX_FD(evbase, event)                                     \
do{                                                                      \
    if(event->ev_fd == evbase->maxfd) evbase->maxfd = event->ev_fd -1;  \
}while(0)
#endif
#ifndef _TYPEDEF_EVENT
#define _TYPEDEF_EVENT
typedef struct _EVENT
{
	int ev_flags;
	int old_ev_flags;
	int ev_fd;
    int bits;
	struct timeval tv;

    void *mutex;
	struct _EVBASE *ev_base;
	void *ev_arg;
	void (*ev_handler)(int fd, int flags, void *arg);	
}EVENT;
/* Set event */
void event_set(EVENT *event, int fd, int flags, void *arg, void *handler);
/* Add event */
void event_add(EVENT *event, int flags);
/* Delete event */
void event_del(EVENT *event, int flags);
/* Active event */
void event_active(EVENT *event, int ev_flags);
/* Destroy event */
void event_destroy(EVENT *event);
/* Clean event */
void event_clean(EVENT *event);
#endif 

#ifdef __cplusplus
 }
#endif

#endif
