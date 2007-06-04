#include "evepoll.h"
#include <sys/epoll.h>
/* Initialize evepoll  */
int evepoll_init(EVBASE *evbase)
{
	 struct rlimit rlim;
        int max_fd = 0;
        if(evbase)
        {
                if(getrlimit(RLIMIT_NOFILE, &rlim) == 0
                        && rlim.rlim_cur != RLIM_INFINITY )
                {
                        max_fd = rlim.rlim_cur;
                }
                else
                {
                        max_fd = EV_MAX_FD;
                }
                evbase->evlist  = (EVENT **)calloc(max_fd, sizeof(EVENT *));
		evbase->efd 	= epoll_create(max_fd);
		FD_CLOSEONEXEC(evbase->fd);
		evbase->evs = calloc(max_fd, sizeof(struct epoll_event));
                return 0;
        }
}
/* Add new event to evbase */
int evepoll_add(EVBASE *evbase, EVENT *event)
{
	int op = 0;
	struct epoll_event ep_event = {0, {0}};
	int ev_flags = 0;
	if(evbase && event)
	{
		/* Delete OLD garbage */
		//epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, NULL);
		memset(evbase->evs[event->fd], 0, sizeof(struct epoll_event));
		ep_event.data.ptr = (void *)event;
		if(event->ev_flags & EV_READ)
		{
			op = EPOLL_CTL_ADD;
			ev_flags |= EPOLLIN;
		}	
		if(event->ev_flags & EV_WRITE)
                {
                        op = EPOLL_CTL_ADD;
                        ev_flags |= EPOLLOUT;
                }
		epoll_ctl(ebase->efd, op, event->ev_fd, &ep_event)	
		if(event->ev_fd > evbase->maxfd)
                        evbase->maxfd = event->ev_fd;
                evbase->evlist[event->ev_fd] = event;
		return 0;	
	}
	return -1;
}
/* Update event in evbase */
int evepoll_update(EVBASE *evbase, EVENT *event)
{
        int op = 0;
        struct epoll_event ep_event = {0, {0}};
        int ev_flags = 0;
        if(evbase && event)
        {
                //memset(evbase->evs[event->fd], 0, sizeof(struct epoll_event));
                /* Delete OLD garbage */
                //epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, NULL);
                ep_event.data.ptr = (void *)event;
                if(event->ev_flags & EV_READ)
                {
                        op = EPOLL_CTL_ADD;
                        ev_flags |= EPOLLIN;
                }
                if(event->ev_flags & EV_WRITE)
                {
                        op = EPOLL_CTL_ADD;
                        ev_flags |= EPOLLOUT;
                }
                epoll_ctl(ebase->efd, op, event->ev_fd, &ep_event)
                if(event->ev_fd > evbase->maxfd)
                        evbase->maxfd = event->ev_fd;
                evbase->evlist[event->ev_fd] = event;
                return 0;
        }
        return -1;	
}
/* Delete event from evbase */
int evepoll_del(EVBASE *evbase, EVENT *event)
{
	struct epoll_event ep_event = {0, {0}};
	if(evbase && event)
	{
		epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, &ep_event);		
		memset(evbase->evs[event->fd], 0, sizeof(struct epoll_event));
		if(event->ev_fd == evbase->maxfd)
                        evbase->maxfd--;
                evbase->evlist[event->ev_fd] = NULL;
		return 0;
	}
	return -1;
}

/* Loop evbase */
void evepoll_loop(EVBASE *evbase, short loop_flags, timeval *tv)
{
	int i = 0; n = 0;
	int timeout = 0;
	short ev_flags = 0;
	struct epoll_event ep_event = {0, {0}};
	if(evbase)
	{
		if(tv)tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		n = epoll_wait(evbase->efd, evbase->evs, evbase->ev_maxfd, timeout);
		for(; i < evbase->ev_maxfd; i++)
		{
			if(evbase->evlist[i])	
			{
				if(evbase->evs[i]->events & (EPOLLHUP | EPOLLERR))
					ev_flags |= (EV_READ |EV_WRITE)
				if(evbase->evs[i]->events & EPOLLIN)
					ev_flags |= EV_READ;
				if(evbase->evs[i]->events & EPOLLOUT)
					ev_flags |= EV_WRITE;
				if(evbase->evs[i]->data.ptr == (void *)evbase->evlist[i])
				{
					if((ev_flags &= evbase->evlist[i]->ev_flags))
					{
						evbase->evlist[i]->active(evbase->evlist[i], ev_flags);	
					}
				}
				else
				{
					epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, &ep_event);
				}
			}
		}
	}
}
/* Clean evbase */
void evepoll_clean(EVBASE **evbase)
{
	if(*evbase)
        {
                if((*evbase)->evlist)free((*evbase)->evlist);
                if((*evbase)->evs)free((*evbase)->evs)
                if((*evbase)->ev_fds)free((*evbase)->ev_fds)
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds)
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds)
                (*evbase) = NULL;
        }	
}

