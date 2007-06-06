#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
/* Clean evbase */
void evbase_clean(EVBASE **evbase);

/* Delete event */
void event_del(EVENT *event);
/* Update event */
void event_update(EVENT *event, short flags);
/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void *handler);
/* Active event */
void event_active(EVENT *event, short ev_flags);
/* Clean event */
void event_clean(EVENT **event);
/* Initialize evbase */
EVBASE *evbase_init()
{
	EVBASE *evbase = (EVBASE *)calloc(1, sizeof(EVBASE));	
	if(evbase)
	{
#ifdef HAVE_EVPORT
		evbase->init 	=  evport_init;
		evbase->add 	=  evport_add;
		evbase->update 	=  evport_update;
		evbase->del 	=  evport_del;
		evbase->loop 	=  evport_loop;
		evbase->clean 	=  evport_clean;
		fprintf(stdout, "Using PORT event mode \n");
#endif 
#ifdef HAVE_EVSELECT
		evbase->init 	=  evselect_init;
		evbase->add 	=  evselect_add;
		evbase->update 	=  evselect_update;
		evbase->del 	=  evselect_del;
		evbase->loop 	=  evselect_loop;
		evbase->clean 	=  evselect_clean;
		fprintf(stdout, "Using SELECT event mode \n");
#endif 
#ifdef HAVE_EVPOLL
		evbase->init 	=  evpoll_init;
		evbase->add 	=  evpoll_add;
		evbase->update 	=  evpoll_update;
		evbase->del 	=  evpoll_del;
		evbase->loop 	=  evpoll_loop;
		evbase->clean 	=  evpoll_clean;
		fprintf(stdout, "Using POLL event mode \n");
#endif 
#ifdef HAVE_EVRTSIG
		evbase->init 	=  evrtsig_init;
		evbase->add 	=  evrtsig_add;
		evbase->update 	=  evrtsig_update;
		evbase->del 	=  evrtsig_del;
		evbase->loop 	=  evrtsig_loop;
		evbase->clean	=  evrtsig_clean;
		fprintf(stdout, "Using SIGNAL event mode \n");
#endif 
#ifdef HAVE_EVEPOLL
		evbase->init 	=  evepoll_init;
		evbase->add 	=  evepoll_add;
		evbase->update 	=  evepoll_update;
		evbase->del 	=  evepoll_del;
		evbase->loop 	=  evepoll_loop;
		evbase->clean 	=  evepoll_clean;
		fprintf(stdout, "Using EPOLL event mode \n");
#endif 
#ifdef HAVE_EVKQUEUE
		evbase->init 	=  evkqueue_init;
		evbase->add 	=  evkqueue_add;
		evbase->update 	=  evkqueue_update;
		evbase->del 	=  evkqueue_del;
		evbase->loop 	=  evkqueue_loop;
		evbase->clean 	=  evkqueue_clean;
		fprintf(stdout, "Using KQUEUE event mode \n");
#endif 
#ifdef HAVE_EVDEVPOLL
		evbase->init 	=  evdevpoll_init;
		evbase->add 	=  evdevpoll_add;
		evbase->update 	=  evdevpoll_update;
		evbase->del 	=  evdevpoll_del;
		evbase->loop 	=  evdevpoll_loop;
		evbase->clean 	=  evdevpoll_clean;
		fprintf(stdout, "Using DEVPOLL event mode \n");
#endif 
#ifdef WIN32
		evbase->init 	=  evwin32_init;
		evbase->add 	=  evwin32_add;
		evbase->update 	=  evwin32_update;
		evbase->del 	=  evwin32_del;
		evbase->loop 	=  evwin32_loop;
		evbase->clean 	=  evwin32_clean;
		fprintf(stdout, "Using WIN32 event mode \n");
#endif
		//evbase->clean 	=  evbase_clean;
		if(evbase->init == NULL || evbase->add == NULL || evbase->update == NULL 
				|| evbase->del == NULL || evbase->loop == NULL || evbase->clean == NULL)
		{
			free(evbase); 
			evbase = NULL;
		}
		else
		{
			evbase->init(evbase);
		}
	}
	return evbase;
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
		event->clean	= event_clean;
	}
	return event;
}

/* Set event */
void event_set(EVENT *event, int fd, short flags, void *arg, void *handler)
{
	if(event)
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
	if(event && event->ev_handler)
	{
		event->ev_handler(event->ev_fd, ev_flags, event->ev_arg);	
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
