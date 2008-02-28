#include "evpoll.h"
#include "logger.h"
#include <errno.h>
#ifdef HAVE_EVPOLL
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/resource.h>
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
		evbase->allowed = max_fd;
                return 0;	
	}	
}
/* Add new event to evbase */
int evpoll_add(EVBASE *evbase, EVENT *event)
{
	struct pollfd *ev = NULL;
	short ev_flags = 0;
	if(evbase && event && event->ev_fd > 0 && evbase->ev_fds && evbase->evlist)
	{
		event->ev_base = evbase;
		ev = &(((struct pollfd *)evbase->ev_fds)[event->ev_fd]);
		if(event->ev_flags & E_READ)
                {
			ev_flags |= POLLIN;
                }
                if(event->ev_flags & E_WRITE)
                {
			ev_flags |= POLLOUT;
                }
		if(ev_flags)
		{
			ev->events = ev_flags;
			ev->revents = 0;
			ev->fd	  = event->ev_fd;
			if(event->ev_fd > evbase->maxfd)
				evbase->maxfd = event->ev_fd;
			evbase->evlist[event->ev_fd] = event;	
			DEBUG_LOGGER(evbase->logger, "Added POLL event:%d on %d", event->ev_flags, event->ev_fd);
		}
		return 0;
	}
	return -1;
}
/* Update event in evbase */
int evpoll_update(EVBASE *evbase, EVENT *event)
{
	struct pollfd *ev = NULL;
        short ev_flags = 0;
	if(evbase && event && evbase->ev_fds && event->ev_fd > 0 && event->ev_fd <= evbase->maxfd)
        {
		event->ev_base = evbase;
                ev = &(((struct pollfd *)evbase->ev_fds)[event->ev_fd]);
                if(event->ev_flags & E_READ)
                {
                        ev_flags |= POLLIN;
                }
                if(event->ev_flags & E_WRITE)
                {
                        ev_flags |= POLLOUT;
                }
                if(ev_flags)
                {
                        ev->events = ev_flags;
			ev->revents = 0;
			ev->fd	  = event->ev_fd;
                        if(event->ev_fd > evbase->maxfd)
                                evbase->maxfd = event->ev_fd;
                        evbase->evlist[event->ev_fd] = event;
			evbase->nfd++;
                        DEBUG_LOGGER(evbase->logger, "Updated POLL event:%d on %d", 
				event->ev_flags, event->ev_fd);
			return 0;
                }
        }	
	return -1;
}
/* Delete event from evbase */
int evpoll_del(EVBASE *evbase, EVENT *event)
{
	if(evbase && event && evbase->ev_fds && event->ev_fd > 0 && event->ev_fd <= evbase->maxfd)
        {
		memset(&(((struct pollfd *)evbase->ev_fds)[event->ev_fd]), 0, sizeof(struct pollfd));
		if(event->ev_fd >= evbase->maxfd)
                        evbase->maxfd = event->ev_fd - 1;
		evbase->evlist[event->ev_fd] = NULL;
		evbase->nfd--;
        }	
}
/* Loop evbase */
void evpoll_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
	int sec = 0;
	int n = 0, i = 0;
	short ev_flags = 0;
	struct pollfd *ev = NULL;
	if(evbase && evbase->ev_fds && evbase->nfd > 0)
	{	
		if(tv) sec = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		n = poll(evbase->ev_fds, evbase->maxfd + 1 , sec);		
		if(n == -1)
                {
                        FATAL_LOGGER(evbase->logger, "Looping evbase[%08x] error[%d], %s", 
				evbase, errno, strerror(errno));
                }
		if(n <= 0) return ;
		//DEBUG_LOG("Actived %d event in %d", n,  evbase->maxfd + 1);
		for(; i <= evbase->maxfd; i++)
		{
			ev = &(((struct pollfd *)evbase->ev_fds)[i]);
			if(evbase->evlist[i] && ev->fd > 0)
			{
				ev_flags = 0;
				if(ev->revents & POLLIN)
				{
					ev_flags |= E_READ;
				}
				if(ev->revents & POLLOUT)	
				{
					ev_flags |= E_WRITE;
				}
				if(ev->revents & (POLLHUP|POLLERR))	
				{
					ev_flags |= (E_READ|E_WRITE);
				}
				if((ev_flags  &= evbase->evlist[i]->ev_flags))
                                {
                                        evbase->evlist[i]->active(evbase->evlist[i], ev_flags);
                                }
			}
		}
	}
}

/* Reset evbase */
void evpoll_reset(EVBASE *evbase)
{
}

/* Clean evbase */
void evpoll_clean(EVBASE **evbase)
{
	if(*evbase)
        {
                if((*evbase)->logger)CLOSE_LOGGER((*evbase)->logger);
                if((*evbase)->evlist)free((*evbase)->evlist);
                if((*evbase)->ev_fds)free((*evbase)->ev_fds);
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
		free(*evbase);
                (*evbase) = NULL;
        }
}
#endif
