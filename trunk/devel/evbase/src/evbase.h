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
#define EV_USEC_SLEEP	100
struct _EVENT;
#ifndef _TYPEDEF_EVBASE
#define _TYPEDEF_EVBASE
typedef struct _EVBASE
{
	void *ev_read_fds;
	void *ev_write_fds;
	void *ev_fds;
	void *ev_changes;
	void *evs;
	int  efd;

	int  maxfd;
	struct _EVENT **evlist;
	int usec_sleep ;

	int	(*init)(struct _EVBASE *);
	int	(*add)(struct _EVBASE *, struct _EVENT*);
	int 	(*update)(struct _EVBASE *, struct _EVENT*);
	int 	(*del)(struct _EVBASE *, struct _EVENT*);
	void	(*loop)(struct _EVBASE *, short , struct timeval *tv);
	void 	(*clean)(struct _EVBASE **);
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
	void (*set)(struct _EVENT *, int fd, short event, void *arg, void *);
	void (*add)(struct _EVENT *, short flags);
	void (*del)(struct _EVENT *, short flags);
	void (*active)(struct _EVENT *, short flags);
	void (*destroy)(struct _EVENT *);
	void (*clean)(struct _EVENT **);
}EVENT;
EVENT *ev_init();
#endif 

#ifdef __cplusplus
 }
#endif

#endif
