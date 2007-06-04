#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifndef _EVENT_H
#define _EVENT_H
#define EV_MAX_FD	65535
#define EV_USEC_SLEEP	100
struct _EVENT;
#ifndef _TYPEDEF_EVBASE
#define _TYPEDEF_EVBASE
typedef struct _EVBASE
{
#ifdef HAVE_PTHREAD_H
	pthread_mutex_t mutex;
#endif	
	void *ev_read_fds;
	void *ev_write_fds;
	void *ev_fds;
	void *evs;
	int  ev_fd;

	int  maxfds;
	struct _EVENT **evlist;
	int usec_sleep ;

	int	(*add)(struct _EVBASE *, struct _EVENT*);
	int 	(*update)(struct _EVBASE *, struct _EVENT*);
	int 	(*del)(struct _EVBASE *, struct _EVENT*);
	void	(*loop)(struct _EVBASE *, short , timeval *tv);
	int 	(*clean)(struct _EVBASE **);
}EVBASE;
EVBASE *evbase_init();
#endif
#ifndef _TYPEDEF_EVENT
#define _TYPEDEF_EVENT
typedef struct _EVENT
{
	struct timeval tv;
	short ev_flags;
	int ev_fd;
	void *ev_arg;
	struct _EVBASE *ev_base;
	void (*ev_handler)(int fd, short flags, void *arg);	
	void (*set)(struct _EVENT *, int fd, short event, void *arg);
	void (*update)(struct _EVENT *, short flags);
	void (*del)(struct _EVENT *);
	void (*active)(struct _EVENT *, short flags);
	void (*clean)(struct _EVENT **);
}EVENT;
EVENT *event_init();
#endif 
#endif
