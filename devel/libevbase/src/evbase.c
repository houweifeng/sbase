#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
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
        evbase->logger = (LOGGER *)logger_init(file);
    }
    return ;
}
/* Initialize evbase */
EVBASE *evbase_init()
{
	EVBASE *evbase = (EVBASE *)calloc(1, sizeof(EVBASE));	
	char *s = NULL;
	if(evbase)
	{
        evbase->set_logfile = evbase_set_logfile;
#ifdef HAVE_EVPORT
		evbase->init 	=  evport_init;
		evbase->add 	=  evport_add;
		evbase->update 	=  evport_update;
		evbase->del 	=  evport_del;
		evbase->loop 	=  evport_loop;
		evbase->reset 	=  evport_reset;
		evbase->clean 	=  evport_clean;
		s = "Using PORT event mode";
#endif 
#ifdef HAVE_EVSELECT
		evbase->init 	=  evselect_init;
		evbase->add 	=  evselect_add;
		evbase->update 	=  evselect_update;
		evbase->del 	=  evselect_del;
		evbase->loop 	=  evselect_loop;
		evbase->reset 	=  evselect_reset;
		evbase->clean 	=  evselect_clean;
		s = "Using SELECT event mode";
#endif 
#ifdef HAVE_EVPOLL
		evbase->init 	=  evpoll_init;
		evbase->add 	=  evpoll_add;
		evbase->update 	=  evpoll_update;
		evbase->del 	=  evpoll_del;
		evbase->loop 	=  evpoll_loop;
		evbase->reset 	=  evpoll_reset;
		evbase->clean 	=  evpoll_clean;
		s = "Using POLL event mode";
#endif 
#ifdef N_HAVE_EVRTSIG
		evbase->init 	=  evrtsig_init;
		evbase->add 	=  evrtsig_add;
		evbase->update 	=  evrtsig_update;
		evbase->del 	=  evrtsig_del;
		evbase->loop 	=  evrtsig_loop;
		evbase->reset 	=  evrtsig_reset;
		evbase->clean	=  evrtsig_clean;
		s = "Using SIGNAL event mode";
#endif 
#ifdef HAVE_EVEPOLL
		evbase->init 	=  evepoll_init;
		evbase->add 	=  evepoll_add;
		evbase->update 	=  evepoll_update;
		evbase->del 	=  evepoll_del;
		evbase->loop 	=  evepoll_loop;
		evbase->reset 	=  evepoll_reset;
		evbase->clean 	=  evepoll_clean;
		s = "Using EPOLL event mode";
#endif 
#ifdef HAVE_EVKQUEUE
		evbase->init 	=  evkqueue_init;
		evbase->add 	=  evkqueue_add;
		evbase->update 	=  evkqueue_update;
		evbase->del 	=  evkqueue_del;
		evbase->loop 	=  evkqueue_loop;
		evbase->reset 	=  evkqueue_reset;
		evbase->clean 	=  evkqueue_clean;
		s = "Using KQUEUE event mode";
#endif 
#ifdef HAVE_EVDEVPOLL
		evbase->init 	=  evdevpoll_init;
		evbase->add 	=  evdevpoll_add;
		evbase->update 	=  evdevpoll_update;
		evbase->del 	=  evdevpoll_del;
		evbase->loop 	=  evdevpoll_loop;
		evbase->reset 	=  evdevpoll_reset;
		evbase->clean 	=  evdevpoll_clean;
		s = "Using DEVPOLL event mode";
#endif 
#ifdef WIN32
		evbase->init 	=  evwin32_init;
		evbase->add 	=  evwin32_add;
		evbase->update 	=  evwin32_update;
		evbase->del 	=  evwin32_del;
		evbase->loop 	=  evwin32_loop;
		evbase->reset 	=  evwin32_reset;
		evbase->clean 	=  evwin32_clean;
		s = "Using WIN32 event mode";
#endif
		fprintf(stdout, "%s\n", s);
		//evbase->clean 	=  evbase_clean;
		if(evbase->init == NULL || evbase->add == NULL || evbase->update == NULL 
				|| evbase->del == NULL || evbase->loop == NULL 
				|| evbase->reset == NULL || evbase->clean == NULL
                ||  evbase->init(evbase) != 0)
		{
			free(evbase); 
			fprintf(stderr, "Initialize evbase failed, %s", strerror(errno));
			evbase = NULL;
		}
	}
	return evbase;
}

/* Initialize event */
EVENT *ev_init()
{
	EVENT *event = (EVENT *)calloc(1, sizeof(EVENT));
	if(event)
	{
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
}

/* Add event flags */
void event_add(EVENT *event, short flags)
{
	if(event)
	{
		event->ev_flags |= flags;
		if(event->ev_base && event->ev_base->update)
		{
			event->ev_base->update(event->ev_base, event);
			DEBUG_LOGGER(event->ev_base->logger, 
                    "Updated event[%08x] to %d on %d",
				event, event->ev_flags, event->ev_fd);
		}
	}
}

/* Delete event flags */
void event_del(EVENT *event, short flags)
{
	if(event)
	{
		if(event->ev_flags & flags)
		{
			event->ev_flags ^= flags;
			if(event->ev_base && event->ev_base->update)
			{
				event->ev_base->update(event->ev_base, event);
				DEBUG_LOGGER(event->ev_base->logger, "Updated event[%08x] to %d on %d",
					event, event->ev_flags, event->ev_fd);
			}

		}
	}	
}

/* Destroy event */
void event_destroy(EVENT *event)
{
        if(event)
        {
                event->ev_flags = 0;
                if(event->ev_base && event->ev_base->del)
                {
                        event->ev_base->del(event->ev_base, event);
                        DEBUG_LOGGER(event->ev_base->logger, "Destroy event[%08x] on %d",
				event, event->ev_fd);
			event->ev_base = NULL;
                }
        }
}

/* Active event */
void event_active(EVENT *event, short ev_flags)
{
	int n = 16;
	char buf[16];
	short e_flags = ev_flags;
	if(event && event->ev_handler)
	{
		DEBUG_LOGGER(event->ev_base->logger, 
			"Activing event[%d] on %d ", event->ev_fd, e_flags);
		event->ev_handler(event->ev_fd, e_flags, event->ev_arg);	
		if(!(event->ev_flags & E_PERSIST))
		{
			event->destroy(event);
		}
	}
}

/* Clean event */
void event_clean(EVENT **event)
{
	if(*event)
	{
		free((*event));
		(*event) = NULL;	
	}
}
