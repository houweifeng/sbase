#include "evpoll.h"
#ifdef HAVE_POLL
#include <poll.h>
#include <resource.h>
/* Initialize evpoll  */
int evpoll_init(EVBASE *evbase)
{
	struct rlimit rlim;
        int max_fd = EV_MAX_FD;
	if(evbase)
	{
		if(getrlimit(RLIMIT_NOFILE, &rlim) == 0
                        && rlim.rlim_cur != RLIM_INFINITY )
                {
                        max_fd = rlim.rlim_cur;
                }
                evbase->evlist = (EVENT **)calloc(max_fd, sizeof(EVENT *));
		evbase->ev_fds = calloc(max_fd, sizeof(struct pollfd));
                return 0;	
	}	
}
/* Add new event to evbase */
int evpoll_add(EVBASE *evbase, EVENT *event)
{
	if(evbase && event && evbase->ev_fds)
	{
		if(event->ev_flags & EV_READ)
                {
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events &= POLLIN;
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->fd  = event->ev_fd;
                }
                if(event->ev_flags & EV_WRITE)
                {
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events &= POLLOUT;
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->fd  = event->ev_fd;
                }
                if(event->ev_fd > evbase->maxfd)
                        evbase->maxfd = event->ev_fd;
                evbase->evlist[event->ev_fd] = event;		
	}	
}
/* Update event in evbase */
int evpoll_update(EVBASE *evbase, EVENT *event)
{
	if(evbase && event && evbase->ev_fds)
        {
		((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events = 0;
                if(event->ev_flags & EV_READ)
                {
                        ((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events &= POLLIN;
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->fd  = event->ev_fd;
                }
                if(event->ev_flags & EV_WRITE)
                {
                        ((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events &= POLLOUT;
			((struct pollfd *)evbase->ev_fds)[event->ev_fd]->fd  = event->ev_fd;
                }
                if(event->ev_fd > evbase->maxfd)
                        evbase->maxfd = event->ev_fd;
                evbase->evlist[event->ev_fd] = event; 
        }	
}
/* Delete event from evbase */
int evpoll_del(EVBASE *evbase, EVENT *event)
{
	if(evbase && event && evbase->ev_fds)
        {
                ((struct pollfd *)evbase->ev_fds)[event->ev_fd]->events = 0;
		((struct pollfd *)evbase->ev_fds)[event->ev_fd]->fd  = -1;
		if(event->ev_fd >= evbase->maxfd)
                        evbase->maxfd = event->ev_fd - 1;
		evbase->evlist[event->ev_fd] = NULL;
        }	
}
/* Loop evbase */
void evpoll_loop(EVBASE *evbase, short, timeval *tv)
{
	int sec = 0;
	int n = 0, i = 0;
	short ev_flags = 0;
	if(evbase && evbase->ev_fds)
	{	
		sec = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		n = poll(evbase->ev_fds, evbase->ev_maxfd, sec);		
		for(; i < evbase->ev_maxfd; i++)
		{
			if(evbase->evlist[i] && ((struct pollfd *)evbase->ev_fds)[i]->fd > 0)
			{
				if(((struct pollfd *)evbase->ev_fds)[i]->revent & POLLIN)
				{
					ev_flags |= EV_READ;
				}
				if(((struct pollfd *)evbase->ev_fds)[i]->revent & POLLOUT)	
				{
					ev_flags |= EV_WRITE;
				}
				if(((struct pollfd *)evbase->ev_fds)[i]->revent & (POLLHUP|POLLERR))	
				{
					ev_flags |= (EV_READ|EV_WRITE);
				}
				if((ev_flags  &= evbase->evlist[i]->ev_flags))
                                {
                                        evbase->evlist[i]->active(evbase->evlist[i], ev_flags);
                                }
			}
		}
	}
}
/* Clean evbase */
void evpoll_clean(EVBASE **evbase)
{
	if(*evbase)
        {
                if((*evbase)->evlist)free((*evbase)->evlist);
                if((*evbase)->ev_fds)free((*evbase)->ev_fds)
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds)
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds)
                (*evbase) = NULL;
        }
}
#endif
