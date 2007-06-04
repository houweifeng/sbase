#include "evselect.h"
/* Initialize evselect  */
int evselect_init(EVBASE *evbase)
{
	struct rlimit rlim;
	int max_fd = 0;
	if(evbase)
	{
		evbase->ev_read_fds = calloc(1, sizeof(fd_set));			
		FD_ZERO((fd_set *)evbase->ev_read_fds);
		FD_SET(0, (fd_set *)evbase->ev_read_fds);
		evbase->ev_write_fds = calloc(1, sizeof(fd_set));	
		FD_ZERO((fd_set *)evbase->ev_write_fds);
		FD_SET(0, (fd_set *)evbase->ev_write_fds);	
		if(getrlimit(RLIMIT_NOFILE, &rlim) == 0
            		&& rlim.rlim_cur != RLIM_INFINITY )
		{
			max_fd = rlim.rlim_cur;		
		}
		else
		{
			max_fd = EV_MAX_FD;
		}
		evbase->evlist = (EVENT **)calloc(max_fd, sizeof(EVENT *));	
		return 0;
	}
	return -1;
}
/* Add new event to evbase */
int evselect_add(EVBASE *evbase, EVENT *event)
{
	if(evbase && event)
	{
		if( evbase->ev_read_fds && (event->ev_flags & EV_READ))
		{
			FD_SET(event->ev_fd, (fd_set *)evbase->ev_read_fds);	
		}
		if(evbase->ev_read_fds && (event->ev_flags & EV_WRITE))
		{
			FD_SET(event->ev_fd, (fd_set *)evbase->ev_write_fds);	
		}
		if(event->ev_fd > evbase->maxfd) 
			evbase->maxfd = event->ev_fd;
		evbase->evlist[event->ev_fd] = event;	
		return 0;
	}	
	return -1;
}
/* Update event in evbase */
int evselect_update(EVBASE *evbase, EVENT *event)
{
	if(evbase && event)
        {
		FD_CLR(event->ev_fd, (fd_set *)evbase->ev_read_fds);
		FD_CLR(event->ev_fd, (fd_set *)evbase->ev_write_fds);
                if(event->ev_flags & EV_READ)
                {
                        FD_SET(event->ev_fd, (fd_set *)evbase->ev_read_fds);
                }
                if(event->ev_flags & EV_WRITE)
                {
                        FD_SET(event->ev_fd, (fd_set *)evbase->ev_write_fds);
                }
		if(event->ev_fd > evbase->maxfd)
                        evbase->maxfd = event->ev_fd;
		evbase->evlist[event->ev_fd] = event;	
		return 0;
        }
	return -1;
}
/* Delete event from evbase */
int evselect_del(EVBASE *evbase, EVENT *event)
{
	if(evbase && event)
        {
                FD_CLR(event->ev_fd, (fd_set *)evbase->ev_read_fds);
                FD_CLR(event->ev_fd, (fd_set *)evbase->ev_write_fds);
		if(event->ev_fd == evbase->maxfd)
                        evbase->maxfd--;
		evbase->evlist[event->ev_fd] = NULL;	
		return 0;
        }
	return -1;
}

/* Loop evbase */
void evselect_loop(EVBASE *evbase, short loop_flag, timeval *tv)
{
	int i = 0, n = 0;
	short ev_flags = 0;
	if(evbase)
	{
		n = select(evbase->maxfds + 1, evbase->ev_read_fds, evbase->ev_write_fds, NULL, tv);
		for(; i < evbase->maxfds; i++)
		{
			if(evbase->evlist[i])
			{
				if(FD_ISSET(evbase->evlist[i], evbase->ev_read_fds))	
				{
					ev_flags |= EV_READ;
				}
				if(FD_ISSET(evbase->evlist[i], evbase->ev_write_fds))
                                {
                                        ev_flags |= EV_WRITE;
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
void evselect_clean(EVBASE **evbase)
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

