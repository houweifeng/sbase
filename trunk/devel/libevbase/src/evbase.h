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
#ifndef _TYPEDEF_EVBASE
#define _TYPEDEF_EVBASE
typedef struct _EVBASE
{
	void *ev_read_fds;
	void *ev_write_fds;
	void *ev_fds;
	void *evs;
	int  efd;
	void *logger;
    void *mutex;

	int nfd;
    int nevent;
	int  maxfd;
	struct _EVENT **evlist;
	int usec_sleep ;
	int allowed;
	int state;

    void    (*set_logfile)(struct _EVBASE *, char *logfile);
    int     (*set_evops)(struct _EVBASE *, int evopid);
	int	    (*init)(struct _EVBASE *);
	int	    (*add)(struct _EVBASE *, struct _EVENT*);
	int 	(*update)(struct _EVBASE *, struct _EVENT*);
	int 	(*del)(struct _EVBASE *, struct _EVENT*);
	int	    (*loop)(struct _EVBASE *, short , struct timeval *tv);
	void	(*reset)(struct _EVBASE *);
	void 	(*clean)(struct _EVBASE **);
}EVBASE;
EVBASE *evbase_init(int use_mutex);
#endif
#ifndef _TYPEDEF_EVENT
#define _TYPEDEF_EVENT
typedef struct _EVENT
{
	struct timeval tv;
	short ev_flags;
	short old_ev_flags;
	int ev_fd;
	void *ev_arg;
    void *mutex;
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
