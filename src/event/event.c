#include "event.h"
#ifdef HAVE_EVENT_PORTS
#include "evports.h"
#endif
#ifdef HAVE_SELECT
#include "evselect.h"
#endif
#ifdef HAVE_POLL
#include "evpoll.h"
#endif
#ifdef HAVE_RTSIG
#include "evrtsig.h"
#endif
#ifdef HAVE_EPOLL
#include "evepoll.h"
#endif
#ifdef HAVE_WORKING_KQUEUE
#include "evkqueue.h"
#endif
#ifdef HAVE_DEVPOLL
#include "evdevpoll.h"
#endif
#ifdef WIN32
#include "evwin32.h"
#endif
/* Clean evbase */
void evbase_clean(EVBASE **evbase);

/* Delete event */
void event_del(EVENT *event);
/* Update event */
void event_update(EVENT *event, short flags);
/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void (*handler)(int, short, void *));
/* Clean event */
void event_clean(EVENT **event);
/* Initialize evbase */
EVBASE *evbase_init()
{
	EVBASE *evbase = (EVBASE *)calloc(1, sizeof(EVBASE));	
	if(evbase)
	{
#ifdef HAVE_EVENT_PORTS
			evbase->init 	=  evports_init;
			evbase->add 	=  evports_add;
			evbase->update 	=  evports_update;
			evbase->del 	=  evports_del;
			evbase->loop 	=  evports_loop;
			evbase->clean 	=  evports_clean;
#endif 
#ifdef HAVE_SELECT
			evbase->init 	=  evselect_init;
			evbase->add 	=  evselect_add;
			evbase->update 	=  evselect_update;
			evbase->del 	=  evselect_del;
			evbase->loop 	=  evselect_loop;
			evbase->clean 	=  evselect_clean;
#endif 
#ifdef HAVE_POLL
			evbase->init 	=  evpoll_init;
			evbase->add 	=  evpoll_add;
			evbase->update 	=  evpoll_update;
			evbase->del 	=  evpoll_del;
			evbase->loop 	=  evpoll_loop;
			evbase->clean 	=  evpoll_clean;
#endif 
#ifdef HAVE_RTSIG
			evbase->init 	=  evrtsig_init;
			evbase->add 	=  evrtsig_add;
			evbase->update 	=  evrtsig_update;
			evbase->del 	=  evrtsig_del;
			evbase->loop 	=  evrtsig_loop;
			evbase->clean	=  evrtsig_clean;
#endif 
#ifdef HAVE_EPOLL
			evbase->init 	=  evepoll_init;
			evbase->add 	=  evepoll_add;
			evbase->update 	=  evepoll_update;
			evbase->del 	=  evepoll_del;
			evbase->loop 	=  evepoll_loop;
			evbase->clean 	=  evepoll_clean;

#endif 
#ifdef HAVE_WORKING_KQUEUE
			evbase->init 	=  evkqueue_init;
			evbase->add 	=  evkqueue_add;
			evbase->update 	=  evkqueue_update;
			evbase->del 	=  evkqueue_del;
			evbase->loop 	=  evkqueue_loop;
			evbase->clean 	=  evkqueue_clean;
#endif 
#ifdef HAVE_DEVPOLL
			evbase->init 	=  evdevpoll_init;
			evbase->add 	=  evdevpoll_add;
			evbase->update 	=  evdevpoll_update;
			evbase->del 	=  evdevpoll_del;
			evbase->loop 	=  evdevpoll_loop;
			evbase->clean 	=  evdevpoll_clean;
#endif 
#ifdef WIN32
			evbase->init 	=  evwin32_init;
			evbase->add 	=  evwin32_add;
			evbase->update 	=  evwin32_update;
			evbase->del 	=  evwin32_del;
			evbase->loop 	=  evwin32_loop;
			evbase->clean 	=  evwin32_clean;
#endif
		//evbase->clean 	=  evbase_clean;
	}
	if(evbase->init == NULL || evbase->add == NULL || evbase->update == NULL 
		|| evbase->del == NULL || evbase->loop == NULL || evbase->clean == NULL)
	{
		free(evbase); 
		evbase = NULL;
	}
	evbase->init(evbase);
	return evbase;
}

/* Loop evbase */
void evbase_loop(EVBASE *evbase, int flags)
{
	
}

/* Clean evbase */
void evbase_clean(EVBASE **evbase)
{
	if((*evbase))
	{
		free((*evbase));
		(*evbase) = NULL;
	}	
}

/* Initialize event */
EVENT *event_init()
{
	EVENT *event = (EVENT *)calloc(1, sizeof(EVENT));
	if(event)
	{
		event->set 	= event_set;
		event->update	= event_update;
		event->del	= event_del;
		event->active	= event_active;
		event->clena	= event_clean;
	}
	return event;
}

/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void *(handler(int, short, void *)))
{
	if(evnet)
	{
		if(fd > 0 && arg && handler)
		{
			event->ev_fd		= 	fd;
			event->ev_flags		=	flags;
			event->ev_arg		=	arg;
			event->ev_handler	=	handler;
		}		
	}
}

/* Update event */
void event_update(EVENT *event, short flags)
{
	if(event)
	{
		event->ev_flags = flags;	
		if(event->ev_base && event->ev_base->update)
			event->ev_base->update(event->ev_base, event);
	}	
}

/* Delete event */
void event_del(EVENT *event)
{
	if(event)
	{
		event->ev_flags = 0;
		if(event->ev_base && event->ev_base->del)
                        event->ev_base->del(event->ev_base, event);
	}	
}

/* Active event */
void event_active(EVENT *event, short ev_flags)
{
	if(event && event->handler)
	{
		event->handler(event->fd, ev_flags, event->arg);	
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

/* TEST server */
#ifdef _DEBUG_EV_SERVER

#endif

/* TEST client */
#ifdef _DEBUG_EV_CLIENT

#endif
