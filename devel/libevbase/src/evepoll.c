#include "evepoll.h"
#include "logger.h"
#include <errno.h>
#ifdef HAVE_EVEPOLL
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/resource.h>
/* Initialize evepoll  */
int evepoll_init(EVBASE *evbase)
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
        evbase->evlist  = (EVENT **)calloc(max_fd, sizeof(EVENT *));
        evbase->efd 	= epoll_create(max_fd);
        evbase->evs 	= calloc(max_fd, sizeof(struct epoll_event));
        evbase->allowed = max_fd;
        return 0;
    }
}

/* Add new event to evbase */
int evepoll_add(EVBASE *evbase, EVENT *event)
{
	int op = 0;
	struct epoll_event ep_event = {0, {0}};
	int ev_flags = 0;
	if(evbase && event && event->ev_fd > 0 )
	{
		event->ev_base = evbase;
		/* Delete OLD garbage */
		//epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, NULL);
		memset(&(((struct epoll_event *)evbase->evs)[event->ev_fd]), 
			0, sizeof(struct epoll_event));
		if(event->ev_flags & E_READ)
		{
			op = EPOLL_CTL_ADD;
			ev_flags |= EPOLLIN;
		}	
		if(event->ev_flags & E_WRITE)
                {
                        op = EPOLL_CTL_ADD;
                        ev_flags |= EPOLLOUT;
                }
		if(ev_flags)
		{
			ep_event.data.fd = event->ev_fd;
			ep_event.events = ev_flags;
			ep_event.data.ptr = (void *)event;
			epoll_ctl(evbase->efd, op, event->ev_fd, &ep_event);
			DEBUG_LOGGER(evbase->logger, "Added event %d on %d", 
				ev_flags, event->ev_fd);
			if(event->ev_fd > evbase->maxfd)
				evbase->maxfd = event->ev_fd;
			evbase->evlist[event->ev_fd] = event;
			evbase->nfd++;
			return 0;	
		}
	}
	return -1;
}

/* Update event in evbase */
int evepoll_update(EVBASE *evbase, EVENT *event)
{
    int op = 0;
    struct epoll_event ep_event = {0, {0}};
    int ev_flags = 0;
    if(evbase && event && event->ev_fd > 0 && event->ev_fd <= evbase->maxfd )
    {
        if(event->ev_flags & E_READ)
        {
            ev_flags |= EPOLLIN;
        }
        if(event->ev_flags & E_WRITE)
        {
            ev_flags |= EPOLLOUT;
        }
        op = EPOLL_CTL_MOD;
        if(ev_flags)
        {
            if(evbase->evlist[event->ev_fd] == NULL)
                op = EPOLL_CTL_ADD;
            ep_event.data.fd = event->ev_fd;
            ep_event.events = ev_flags;
            ep_event.data.ptr = (void *)event;
            epoll_ctl(evbase->efd, op, event->ev_fd, &ep_event);
        }
        evbase->evlist[event->ev_fd] = event;
        return 0;
    }
    return -1;	
}

/* Delete event from evbase */
int evepoll_del(EVBASE *evbase, EVENT *event)
{
    struct epoll_event ep_event = {0, {0}};
    if(evbase && event && event->ev_fd > 0 && event->ev_fd <= evbase->maxfd)
    {
        ep_event.data.fd = event->ev_fd;
        ep_event.events = 0;
        ep_event.data.ptr = (void *)event;
        epoll_ctl(evbase->efd, EPOLL_CTL_DEL, event->ev_fd, &ep_event);
        if(event->ev_fd >= evbase->maxfd)
            evbase->maxfd = event->ev_fd - 1;
        evbase->evlist[event->ev_fd] = NULL;
        evbase->nfd--;
        return 0;
    }
    return -1;
}

/* Loop evbase */
void evepoll_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
	int i = 0, n = 0;
	int timeout = 0;
	short ev_flags = 0;
	struct epoll_event ep_event = {0, {0}};
	int fd = 0;
	struct epoll_event *evp = NULL;
	EVENT *ev = NULL;
	int flags = 0;
	//DEBUG_LOG("Loop evbase[%08x]", evbase);
	//if(evbase)
	if(evbase && evbase->nfd > 0)
	{
		if(tv) timeout = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		memset(evbase->evs, 0, sizeof(struct epoll_event) * evbase->maxfd);
		n = epoll_wait(evbase->efd, evbase->evs, evbase->maxfd, timeout);
		if(n == -1)
		{
			FATAL_LOGGER(evbase->logger, "Looping evbase[%08x] error[%d], %s",
				evbase, errno, strerror(errno));
		}
		if(n <= 0) return ;
		DEBUG_LOGGER(evbase->logger, "Actived %d event in %d", n, evbase->maxfd);
		for(i = 0; i < n; i++)
		{
			evp = &(((struct epoll_event *)evbase->evs)[i]);
			ev = (EVENT *)evp->data.ptr;
			if(ev == NULL)
			{
				FATAL_LOGGER(evbase->logger, "Invalid i:%d evp:%08x event:%08x",
				i, evp, ev);
				continue;
			}
			fd = ev->ev_fd;
			flags = evp->events;
			DEBUG_LOGGER(evbase->logger, "Activing i:%d evp:%08x ev:%08x fd:%d flags:%d",
				i, evp, ev, fd, flags);
			//fd = evp->data.fd;
			if(fd >= 0 && fd <= evbase->maxfd &&  evbase->evlist[fd] 
				&& evp->data.ptr == (void *)evbase->evlist[fd])	
			{
				ev_flags = 0;
				if(flags & EPOLLHUP )
					flags |= (EPOLLIN | EPOLLOUT);
				else if(flags & EPOLLERR )
					flags |= (EPOLLIN | EPOLLOUT);
				if(flags & EPOLLIN)
					ev_flags |= E_READ;
				if(flags & EPOLLOUT)
					ev_flags |= E_WRITE;
				DEBUG_LOGGER(evbase->logger,
					"Activing i:%d evp:%08x ev:%08x fd:%d ev_flags:%d", 
                                	i, evp, ev, fd, ev_flags);
				if((ev_flags &= evbase->evlist[fd]->ev_flags))
				{
					DEBUG_LOGGER(evbase->logger, "Activing EV[%d] on %d", 
						ev_flags, fd);
					evbase->evlist[fd]->active(evbase->evlist[fd], ev_flags);	
					DEBUG_LOGGER(evbase->logger, "Actived EV[%d] on %d", 
						ev_flags, fd);
				}
			}
		}
	}
}

/* Reset evbase */
void evepoll_reset(EVBASE *evbase)
{
	int i = 0;
	if(evbase)
	{
		memset(evbase->evlist, 0, evbase->allowed * sizeof(EVENT *));
		close(evbase->efd);
                memset(evbase->evs, 0, evbase->allowed * sizeof(struct epoll_event));
	}	
}

/* Clean evbase */
void evepoll_clean(EVBASE **evbase)
{
	if(*evbase)
        {
		close((*evbase)->efd);
                if((*evbase)->logger)CLOSE_LOGGER((*evbase)->logger);
                if((*evbase)->evlist)free((*evbase)->evlist);
                if((*evbase)->evs)free((*evbase)->evs);
                if((*evbase)->ev_fds)free((*evbase)->ev_fds);
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
		if((*evbase)->efd > 0 )close((*evbase)->efd);
		free(*evbase);
                (*evbase) = NULL;
        }	
}
#endif
