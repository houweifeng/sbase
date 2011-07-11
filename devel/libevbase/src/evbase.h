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
#ifndef __TYPEDEF__MUTEX
#define __TYPEDEF__MUTEX
#ifdef HAVE_SEMAPHORE
#include <semaphore.h>
typedef struct _MUTEX
{
    sem_t sem;
}MUTEX;
#else
#include <pthread.h>
typedef struct _MUTEX
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int nowait;
}MUTEX;
#endif
#endif
#ifndef _TYPEDEF_EVBASE
#define _TYPEDEF_EVBASE
typedef struct _EVBASE
{
	int  efd;
    int nfd;
    int nevent;
	int  maxfd;
	int usec_sleep ;
	int allowed;
	int state;
    int evopid;
    struct _MUTEX mutex;

	void *ev_read_fds;
	void *ev_write_fds;
	void *ev_fds;
	void *evs;
	void *logger;
    struct _EVENT **evlist;

    void    (*set_logfile)(struct _EVBASE *, char *logfile);
    void    (*set_log_level)(struct _EVBASE *, int level);
    int     (*set_evops)(struct _EVBASE *, int evopid);
	int	    (*init)(struct _EVBASE *);
	int	    (*add)(struct _EVBASE *, struct _EVENT*);
	int 	(*update)(struct _EVBASE *, struct _EVENT*);
	int 	(*del)(struct _EVBASE *, struct _EVENT*);
	int	    (*loop)(struct _EVBASE *, short , struct timeval *tv);
	void	(*reset)(struct _EVBASE *);
	void 	(*clean)(struct _EVBASE *);
}EVBASE;
EVBASE *evbase_init();
#endif
#ifndef _TYPEDEF_EVENT
#define _TYPEDEF_EVENT
typedef struct _EVENT
{
	short ev_flags;
	short old_ev_flags;
	int ev_fd;
	struct timeval tv;
    struct _MUTEX mutex;
	void *ev_arg;
	struct _EVBASE *ev_base;
	void (*ev_handler)(int fd, short flags, void *arg);	
}EVENT;
/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void *handler);
/* Add event */
void event_add(EVENT *event, short flags);
/* Delete event */
void event_del(EVENT *event, short flags);
/* Active event */
void event_active(EVENT *event, short ev_flags);
/* Destroy event */
void event_destroy(EVENT *event);
/* Clean event */
void event_clean(EVENT *event);
#endif 

#ifdef __cplusplus
 }
#endif

#endif
