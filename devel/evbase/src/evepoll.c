#include "evepoll.h"
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
		memset(&(((struct epoll_event *)evbase->evs)[event->ev_fd]), 0, sizeof(struct epoll_event));
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
		ep_event.data.fd = event->ev_fd;
		ep_event.events = ev_flags;
		ep_event.data.ptr = (void *)event;
		epoll_ctl(evbase->efd, op, event->ev_fd, &ep_event);
		fprintf(stdout, "add event %d on %d\n", ev_flags, event->ev_fd);
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
        if(evbase && event && event->ev_fd > 0 && event->ev_fd <= evbase->maxfd )
        {
                //memset(evbase->evs[event->ev_fd], 0, sizeof(struct epoll_event));
                /* Delete OLD garbage */
                //epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, NULL);
                if(event->ev_flags & EV_READ)
                {
                        ev_flags |= EPOLLIN;
                }
                if(event->ev_flags & EV_WRITE)
                {
                        ev_flags |= EPOLLOUT;
                }
		if(ev_flags)
		{
			ep_event.data.fd = event->ev_fd;
			ep_event.events = ev_flags;
			ep_event.data.ptr = (void *)event;
			epoll_ctl(evbase->efd, EPOLL_CTL_MOD, event->ev_fd, &ep_event);
		}
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
	if(evbase)
	{
		if(tv) timeout = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		n = epoll_wait(evbase->efd, evbase->evs, evbase->maxfd, timeout);
		if(n) fprintf(stdout, "loop result:%d\n", n);
		for(i = 0; i < n; i++)
		{
			evp = &(((struct epoll_event *)evbase->evs)[i]);
			ev = (EVENT *)evp->data.ptr;
			fd = ev->ev_fd;
			//fd = evp->data.fd;
			if(evbase->evlist[fd] && evp->data.ptr == (void *)evbase->evlist[fd])	
			{
				ev_flags = 0;
				if(evp->events & (EPOLLHUP | EPOLLERR))
					ev_flags |= (EV_READ |EV_WRITE);
				if(evp->events & EPOLLIN)
					ev_flags |= EV_READ;
				if(evp->events & EPOLLOUT)
					ev_flags |= EV_WRITE;
				if((ev_flags &= evbase->evlist[fd]->ev_flags))
				{
					fprintf(stdout, "EV:%d on %d \n", ev_flags, fd);
					evbase->evlist[fd]->active(evbase->evlist[fd], ev_flags);	
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
                if((*evbase)->evs)free((*evbase)->evs);
                if((*evbase)->ev_fds)free((*evbase)->ev_fds);
                if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
                if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
                (*evbase) = NULL;
        }	
}
#endif
