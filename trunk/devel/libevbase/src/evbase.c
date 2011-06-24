#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <errno.h>
#include "logger.h"
#include "evbase.h"
#ifdef HAVE_EVPORT
#include "evport.h"
#endif
#ifdef HAVE_EVSELECT
#include "evselect.h"
#endif
#ifdef HAVE_EVPOLL
#include "evpoll.h"
#endif
#ifdef HAVE_EVRTSIG
#include "evrtsig.h"
#endif
#ifdef HAVE_EVEPOLL
#include "evepoll.h"
#endif
#ifdef HAVE_EVKQUEUE
#include "evkqueue.h"
#endif
#ifdef HAVE_EVDEVPOLL
#include "evdevpoll.h"
#endif
#ifdef WIN32
#include "evwin32.h"
#endif
#include "xmm.h"
typedef struct _EVOPS
{
    char    *name ;
    int     (*init)(struct _EVBASE *);
    int     (*add)(struct _EVBASE *, struct _EVENT*);
    int     (*update)(struct _EVBASE *, struct _EVENT*);
    int     (*del)(struct _EVBASE *, struct _EVENT*);
    int     (*loop)(struct _EVBASE *, short , struct timeval *tv);
    void    (*reset)(struct _EVBASE *);
    void    (*clean)(struct _EVBASE **);
}EVOPS;
static EVOPS evops[EOP_LIMIT];
static int evops_default =      -1;
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
void event_clean(EVENT **event);

/* set logfile */
void evbase_set_logfile(EVBASE *evbase, char *file)
{
    if(evbase)
    {
        LOGGER_INIT(evbase->logger, file);
        DEBUG_LOGGER(evbase->logger, "Initailize evlog[%s]", file);
    }
    return ;
}
/* set logger level */
void evbase_set_log_level(EVBASE *evbase, int level)
{
    if(evbase && evbase->logger)
    {
        LOGGER_SET_LEVEL(evbase->logger, level);
    }
    return ;
}
/* set event operating */
int evbase_set_evops(EVBASE *evbase, int evopid)
{
    if(evbase)
    {
        if(evopid >= 0 && evopid < EOP_LIMIT && evops[evopid].name != NULL)
        {
            if(evbase->reset)evbase->reset(evbase);
            if(evops[evopid].init) evbase->init = evops[evopid].init;
            if(evops[evopid].add) evbase->add = evops[evopid].add;
            if(evops[evopid].update) evbase->update = evops[evopid].update;
            if(evops[evopid].del) evbase->del = evops[evopid].del;
            if(evops[evopid].loop) evbase->loop = evops[evopid].loop;
            if(evops[evopid].reset) evbase->reset = evops[evopid].reset;
            if(evops[evopid].clean) evbase->clean = evops[evopid].clean;
            if(evbase->init(evbase) == -1)
                evbase->set_evops(evbase, evops_default);
            DEBUG_LOGGER(evbase->logger, "Set event to [%s]", evops[evopid].name);
            return 0;
        }
    }
    return -1;
}

/* Initialize evbase */
EVBASE *evbase_init(int use_mutex)
{
    EVBASE *evbase = NULL;

    if((evbase = (EVBASE *)xmm_new(sizeof(EVBASE))))
    {
        if(use_mutex){MUTEX_INIT(evbase->mutex);}
#ifdef HAVE_EVPORT
        evops_default = EOP_PORT;
        evops[EOP_PORT].name = "PORT";
        evops[EOP_PORT].init      = evport_init;
        evops[EOP_PORT].add       = evport_add;
        evops[EOP_PORT].update    = evport_update;
        evops[EOP_PORT].del       = evport_del;
        evops[EOP_PORT].loop      = evport_loop;
        evops[EOP_PORT].reset     = evport_reset;
        evops[EOP_PORT].clean     = evport_clean;
#endif
#ifdef HAVE_EVSELECT
        evops_default = EOP_SELECT;
        evops[EOP_SELECT].name    = "SELECT";
        evops[EOP_SELECT].init    = evselect_init;
        evops[EOP_SELECT].add     = evselect_add;
        evops[EOP_SELECT].update  = evselect_update;
        evops[EOP_SELECT].del     = evselect_del;
        evops[EOP_SELECT].loop    = evselect_loop;
        evops[EOP_SELECT].reset   = evselect_reset;
        evops[EOP_SELECT].clean   = evselect_clean;
#endif
#ifdef HAVE_EVPOLL
        evops_default = EOP_POLL;
        evops[EOP_POLL].name      = "POLL";
        evops[EOP_POLL].init      = evpoll_init;
        evops[EOP_POLL].add       = evpoll_add;
        evops[EOP_POLL].update    = evpoll_update;
        evops[EOP_POLL].del       = evpoll_del;
        evops[EOP_POLL].loop      = evpoll_loop;
        evops[EOP_POLL].reset     = evpoll_reset;
        evops[EOP_POLL].clean     = evpoll_clean;
#endif
#ifdef HAVE_EVRTSIG
        evops_default = EOP_RTSIG;
        evops[EOP_RTSIG].name     = "RTSIG" ;
        evops[EOP_RTSIG].init     = evrtsig_init;
        evops[EOP_RTSIG].add      = evrtsig_add;
        evops[EOP_RTSIG].update   = evrtsig_update;
        evops[EOP_RTSIG].del      = evrtsig_del;
        evops[EOP_RTSIG].loop     = evrtsig_loop;
        evops[EOP_RTSIG].reset    = evrtsig_reset;
        evops[EOP_RTSIG].clean    = evrtsig_clean;
#endif
#ifdef HAVE_EVEPOLL
        evops_default = EOP_EPOLL;
        evops[EOP_EPOLL].name     = "EPOLL";
        evops[EOP_EPOLL].init     = evepoll_init;
        evops[EOP_EPOLL].add      = evepoll_add;
        evops[EOP_EPOLL].update   = evepoll_update;
        evops[EOP_EPOLL].del      = evepoll_del;
        evops[EOP_EPOLL].loop     = evepoll_loop;
        evops[EOP_EPOLL].reset    = evepoll_reset;
        evops[EOP_EPOLL].clean    = evepoll_clean;
#endif
#ifdef HAVE_EVKQUEUE
        evops_default = EOP_KQUEUE;
        evops[EOP_KQUEUE].name    = "KQUEUE";
        evops[EOP_KQUEUE].init    = evkqueue_init;
        evops[EOP_KQUEUE].add     = evkqueue_add;
        evops[EOP_KQUEUE].update  = evkqueue_update;
        evops[EOP_KQUEUE].del     = evkqueue_del;
        evops[EOP_KQUEUE].loop    = evkqueue_loop;
        evops[EOP_KQUEUE].reset   = evkqueue_reset;
        evops[EOP_KQUEUE].clean   = evkqueue_clean;
#endif
#ifdef HAVE_EVDEVPOLL
        evops_default = EOP_DEVPOLL;
        evops[EOP_DEVPOLL].name   = "/dev/poll";
        evops[EOP_DEVPOLL].init   = evdevpoll_init;
        evops[EOP_DEVPOLL].add    = evdevpoll_add;
        evops[EOP_DEVPOLL].update = evdevpoll_update;
        evops[EOP_DEVPOLL].del    = evdevpoll_del;
        evops[EOP_DEVPOLL].loop   = evdevpoll_loop;
        evops[EOP_DEVPOLL].reset  = evdevpoll_reset;
        evops[EOP_DEVPOLL].clean  = evdevpoll_clean;
#endif
#ifdef WIN32
        evops_default = EOP_WIN32;
        evops[EOP_WIN32].name     = "WIN32";
        evops[EOP_WIN32].init     = evwin32_init;
        evops[EOP_WIN32].add      = evwin32_add;
        evops[EOP_WIN32].update   = evwin32_update;
        evops[EOP_WIN32].del      = evwin32_del;
        evops[EOP_WIN32].loop     = evwin32_loop;
        evops[EOP_WIN32].reset    = evwin32_reset;
        evops[EOP_WIN32].clean    = evwin32_clean;
#endif
        evbase->set_logfile = evbase_set_logfile;
        evbase->set_log_level = evbase_set_log_level;
        evbase->set_evops   = evbase_set_evops;
        //evbase->clean 	=  evbase_clean;
        if(evops_default == -1 || evbase->set_evops(evbase, evops_default) == -1)
        {
            xmm_free(evbase, sizeof(EVBASE)); 
            fprintf(stderr, "Initialize evbase to default[%d] failed, %s\n", 
                    evops_default, strerror(errno));
            evbase = NULL;
        }
    }
    return evbase;
}

/* Initialize event */
EVENT *ev_init()
{
	EVENT *event =  NULL;

	if((event = (EVENT *)xmm_new(sizeof(EVENT))))
	{
        MUTEX_INIT(event->mutex);
		event->set 	= event_set;
		event->add	= event_add;
		event->del	= event_del;
		event->active	= event_active;
		event->destroy	= event_destroy;
		event->clean	= event_clean;
	}
	return event;
}

/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void *handler)
{
	if(event)
	{
		if(fd > 0 && handler)
		{
			event->ev_fd		= 	fd;
			event->ev_flags		=	flags;
			event->ev_arg		=	arg;
			event->ev_handler	=	handler;
		}		
	}
    return ;
}

/* Add event flags */
void event_add(EVENT *event, short flags)
{
	if(event)
	{
        //MUTEX_LOCK(event->mutex);
        event->old_ev_flags = event->ev_flags;
		event->ev_flags |= flags;
		if(event->ev_base && event->ev_base->update)
		{
			DEBUG_LOGGER(event->ev_base->logger, 
                    "Added event[%p] flags[%d] on fd[%d]",
				event, event->ev_flags, event->ev_fd);
			event->ev_base->update(event->ev_base, event);
		}
        //MUTEX_UNLOCK(event->mutex);
	}
    return ;
}

/* Delete event flags */
void event_del(EVENT *event, short flags)
{
	if(event)
	{
        //MUTEX_LOCK(event->mutex);
		if(event->ev_flags & flags)
		{
            event->old_ev_flags = event->ev_flags;
			event->ev_flags ^= flags;
			if(event->ev_base)
			{
                DEBUG_LOGGER(event->ev_base->logger, "Updated event[%p] flags[%d] on fd[%d]",
                            event, event->ev_flags, event->ev_fd);
                if(event->ev_flags & (E_READ|E_WRITE))
                {
                    event->ev_base->update(event->ev_base, event);
                }
                else
                {
                    event->ev_base->del(event->ev_base, event);
                }
			}
		}
        //MUTEX_UNLOCK(event->mutex);
	}	
    return ;
}

/* Destroy event */
void event_destroy(EVENT *event)
{
    if(event)
    {
        //MUTEX_LOCK(event->mutex);
        event->ev_flags = 0;
        if(event->ev_base && event->ev_base->del)
        {
            DEBUG_LOGGER(event->ev_base->logger, "Destroy event[%p] on fd[%d]",event, event->ev_fd);
            event->ev_base->del(event->ev_base, event);
            event->ev_base = NULL;
        }
        //MUTEX_UNLOCK(event->mutex);
    }
    return ;
}

/* Active event */
void event_active(EVENT *event, short ev_flags)
{
	short e_flags = ev_flags;
	if(event)
    {
        //MUTEX_LOCK(event->mutex);
        if(event->ev_handler && event->ev_base && event->ev_flags)
        {
            DEBUG_LOGGER(event->ev_base->logger, 
                    "Activing event[%p] flags[%d] on fd[%d]", event, e_flags, event->ev_fd);
            event->ev_handler(event->ev_fd, e_flags, event->ev_arg);	
        }
        //MUTEX_LOCK(event->mutex);
        if(!(event->ev_flags & E_PERSIST) && event->ev_base)
        {
        	event->destroy(event);
        }
    }
    return ;
}

/* Clean event */
void event_clean(EVENT **event)
{
	if(*event)
	{
        MUTEX_DESTROY((*event)->mutex);
		xmm_free((*event), sizeof(EVENT));
		(*event) = NULL;	
	}
    return ;
}
