#include "evkqueue.h"
#ifdef HAVE_EVKQUEUE
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/resource.h>
/* Initialize evkqueue  */
int evkqueue_init(EVBASE *evbase)
{
	int max_fd = EV_MAX_FD;
	struct rlimit rlim;
	if(evbase)
	{
		if((evbase->efd = kqueue()) == -1) 
			return -1;
		if(getrlimit(RLIMIT_NOFILE, &rlim) == 0
				&& rlim.rlim_cur != RLIM_INFINITY )
		{
			max_fd = rlim.rlim_cur;
		}
		if((evbase->ev_changes	= calloc(max_fd, sizeof(struct kevent))) == NULL ) return -1;
		if((evbase->evs		= calloc(max_fd, sizeof(struct kevent))) == NULL) return -1;
		return 0;
	}
	return -1;
}
/* Add new event to evbase */
int evkqueue_add(EVBASE *evbase, EVENT *event)
{
	struct kevent *kqev = NULL;
	if(evbase && event && evbase->ev_changes && evbase->evs)
	{
		event->ev_base = evbase;
		kqev = &(((struct kevent *)evbase->ev_changes)[event->ev_fd]);
		memset(kqev, 0, sizeof(struct kevent));
		if(event->ev_flags & EV_READ)
		{
			kqev->ident  = event->ev_fd;
			kqev->filter |= EVFILT_READ;
			kqev->flags = EV_ADD|EV_CLEAR;	
			kqev->udata = (void *)event;
		}
		if(event->ev_flags & EV_WRITE)
                {
                        kqev->ident = event->ev_fd;
                        kqev->filter |= EVFILT_WRITE;
                        kqev->flags = EV_ADD|EV_CLEAR;
                        kqev->udata = (void *)event;
                }
		if(event->ev_fd > evbase->maxfd)
			evbase->maxfd = event->ev_fd;
		return 0;
	}
	return -1;
}

/* Update event in evbase */
int evkqueue_update(EVBASE *evbase, EVENT *event)
{
	struct kevent *kqev = NULL;
        if(evbase && event && evbase->ev_changes && evbase->evs)
        {
                kqev = &(((struct kevent *)evbase->ev_changes)[event->ev_fd]);
                memset(kqev, 0, sizeof(struct kevent));
                if(event->ev_flags & EV_READ)
                {
                        kqev->ident  = event->ev_fd;
                        kqev->filter |= EVFILT_READ;
                        kqev->flags = EV_ADD|EV_CLEAR;
                        kqev->udata = (void *)event;
                }
                if(event->ev_flags & EV_WRITE)
                {
                        kqev->ident = event->ev_fd;
                        kqev->filter |= EVFILT_WRITE;
                        kqev->flags = EV_ADD|EV_CLEAR;
                        kqev->udata = (void *)event;
                }
		return 0;
        }
        return -1;
}

/* Delete event from evbase */
int evkqueue_del(EVBASE *evbase, EVENT *event)
{
	struct kevent *kqev = NULL;
        if(evbase && event && evbase->ev_changes && evbase->evs)
        {
                kqev = &(((struct kevent *)evbase->ev_changes)[event->ev_fd]);
                memset(kqev, 0, sizeof(struct kevent));
                if(event->ev_flags & EV_READ)
                {
                        kqev->ident  = event->ev_fd;
                        kqev->filter |= EVFILT_READ;
                        kqev->flags = EV_DELETE|EV_CLEAR;
                        kqev->udata = (void *)event;
                }
                if(event->ev_flags & EV_WRITE)
                {
                        kqev->ident = event->ev_fd;
                        kqev->filter |= EVFILT_WRITE;
                        kqev->flags = EV_DELETE|EV_CLEAR;
                        kqev->udata = (void *)event;
                }
		if(event->ev_fd >= evbase->maxfd)
                        evbase->maxfd = event->ev_fd - 1;
		evbase->evlist[event->ev_fd] = NULL;
		return 0;
        }
        return -1;
}
/* Loop evbase */
void evkqueue_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
	int i = 0, n = 0;
        short ev_flags = 0;	
	struct timespec ts;
	struct kevent *kqev = NULL;
	if(evbase)
	{
		memset(&ts, 0, sizeof(struct timespec));
		if(tv) TIMEVAL_TO_TIMESPEC(tv, &ts);
		n = kevent(evbase->efd, evbase->ev_changes,
			 evbase->maxfd, evbase->evs, evbase->maxfd, &ts);	
		if(n <= 0 ) return ;
		for(i = 0; i < n; i++)
		{
			kqev = &(((struct kevent *)evbase->evs)[i]);
			if(kqev && evbase->evlist[kqev->ident] == kqev->udata)
			{
				if(kqev->filter & EVFILT_READ)	ev_flags |= EV_READ;
				if(kqev->filter & EVFILT_WRITE)	ev_flags |= EV_WRITE;
				if((ev_flags &=  evbase->evlist[kqev->ident]->ev_flags )!= 0) 
					evbase->evlist[kqev->ident]->active(evbase->evlist[kqev->ident], ev_flags);
			}
					
		}
	}
}

/* Clean evbase */
void evkqueue_clean(EVBASE **evbase)
{
	if(*evbase)
        {
                if((*evbase)->evlist)free((*evbase)->evlist);
                if((*evbase)->evs)free((*evbase)->evs);
                if((*evbase)->ev_fds)free((*evbase)->ev_fds);
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
                if((*evbase)->ev_changes)free((*evbase)->ev_changes);
                (*evbase) = NULL;
        }	
}
#endif
