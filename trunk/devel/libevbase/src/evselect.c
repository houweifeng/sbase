#include "evselect.h"
#include "logger.h"
#include <errno.h>
#ifdef HAVE_EVSELECT
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/resource.h>
/* Initialize evselect  */
int evselect_init(EVBASE *evbase)
{
    struct rlimit rlim;
    int max_fd = EV_MAX_FD;
    if(evbase)
    {
        evbase->ev_read_fds = calloc(1, sizeof(fd_set));			
        FD_ZERO((fd_set *)evbase->ev_read_fds);
        //FD_SET(0, (fd_set *)evbase->ev_read_fds);
        evbase->ev_write_fds = calloc(1, sizeof(fd_set));	
        FD_ZERO((fd_set *)evbase->ev_write_fds);
        //FD_SET(0, (fd_set *)evbase->ev_write_fds);	
        if(getrlimit(RLIMIT_NOFILE, &rlim) == 0
                && rlim.rlim_cur != RLIM_INFINITY )
        {
            max_fd = rlim.rlim_cur;		
        }
        evbase->evlist = (EVENT **)calloc(max_fd, sizeof(EVENT *));	
        evbase->allowed = max_fd;
        return 0;
    }
    return -1;
}
/* Add new event to evbase */
int evselect_add(EVBASE *evbase, EVENT *event)
{
    short ev_flags = 0;
    if(evbase && event && event->ev_fd >= 0)
    {
        event->ev_base = evbase;
        if(evbase->ev_read_fds && (event->ev_flags & E_READ))
        {
            FD_SET(event->ev_fd, (fd_set *)evbase->ev_read_fds);
            ev_flags |= E_READ;
        }
        if(evbase->ev_read_fds && (event->ev_flags & E_WRITE))
        {
            ev_flags |= E_WRITE;
            FD_SET(event->ev_fd, (fd_set *)evbase->ev_write_fds);	
        }
        if(ev_flags)
        {
            if(event->ev_fd > evbase->maxfd) 
                evbase->maxfd = event->ev_fd;
            evbase->evlist[event->ev_fd] = event;	
            evbase->nfd++;
            DEBUG_LOGGER(evbase->logger, "Added event[%x] flags[%d] on fd[%d]", 
                    event, ev_flags, event->ev_fd);
            return 0;
        }
    }	
    return -1;
}

/* Update event in evbase */
int evselect_update(EVBASE *evbase, EVENT *event)
{
    short ev_flags = 0, add_ev_flags = 0, del_ev_flags = 0;
    if(evbase && event && event->ev_fd >= 0 && event->ev_fd <= evbase->maxfd)
    {
        ev_flags = (event->ev_flags ^ event->old_ev_flags);
        add_ev_flags = (event->ev_flags & ev_flags);
        del_ev_flags = (event->old_ev_flags & ev_flags);

        if(add_ev_flags & E_READ)
        {
            FD_SET(event->ev_fd, (fd_set *)evbase->ev_read_fds);
            ev_flags |= E_READ;
            DEBUG_LOGGER(evbase->logger, "Added EV_READ on fd[%d]", event->ev_fd);
        }
        if(del_ev_flags & E_READ)
        {
            FD_CLR(event->ev_fd, (fd_set *)evbase->ev_read_fds);
            DEBUG_LOGGER(evbase->logger, "Deleted EV_READ on fd[%d]", event->ev_fd);
        }
        if(add_ev_flags & E_WRITE)
        {
            FD_SET(event->ev_fd, (fd_set *)evbase->ev_write_fds);
            ev_flags |= E_WRITE;
            DEBUG_LOGGER(evbase->logger, "Added EV_WRITE on fd[%d]", event->ev_fd);
        }
        if(del_ev_flags & E_WRITE)
        {
            FD_CLR(event->ev_fd, (fd_set *)evbase->ev_write_fds);
            DEBUG_LOGGER(evbase->logger, "Deleted EV_WRITE on fd[%d]", event->ev_fd);
        }
        if(event->ev_fd > evbase->maxfd)
            evbase->maxfd = event->ev_fd;
        evbase->evlist[event->ev_fd] = event;
        DEBUG_LOGGER(evbase->logger, "Updated event[%08x] flags[%d] on fd[%d]",
                event, event->ev_flags, event->ev_fd);
        return 0;
    }
    return -1;
}
/* Delete event from evbase */
int evselect_del(EVBASE *evbase, EVENT *event)
{
    if(evbase && event && event->ev_fd >= 0 && event->ev_fd <= evbase->maxfd)
    {
        if(evbase->ev_read_fds)
        {
            FD_CLR(event->ev_fd, (fd_set *)evbase->ev_read_fds);
        }
        if(evbase->ev_write_fds)
        {
            FD_CLR(event->ev_fd, (fd_set *)evbase->ev_write_fds);
        }
        DEBUG_LOGGER(evbase->logger, "Deleted event[%08x] flags[%d] on fd[%d]",
                event, event->ev_flags, event->ev_fd);
        if(event->ev_fd >= evbase->maxfd)
            evbase->maxfd = event->ev_fd - 1;
        evbase->evlist[event->ev_fd] = NULL;	
        evbase->nfd--;
        return 0;
    }
    return -1;
}

/* Loop evbase */
void evselect_loop(EVBASE *evbase, short loop_flag, struct timeval *tv)
{
    int i = 0, n = 0;
    short ev_flags = 0;
    fd_set rd_fd_set, wr_fd_set ;

    if(evbase && evbase->nfd  > 0)
    {
        FD_ZERO(&rd_fd_set);
        memcpy(&rd_fd_set, evbase->ev_read_fds, sizeof(fd_set));
        FD_ZERO(&wr_fd_set);
        memcpy(&wr_fd_set, evbase->ev_write_fds, sizeof(fd_set));
        n = select(evbase->maxfd + 1, &rd_fd_set, &wr_fd_set, NULL, tv);
        if(n <= 0) return ;
        DEBUG_LOGGER(evbase->logger, "Actived %d event in %d", n,  evbase->maxfd + 1);
        for(i = 0; i <= evbase->maxfd; ++i)
        {
            if(evbase->evlist[i])
            {
                ev_flags = 0;
                if(FD_ISSET(i, &rd_fd_set))	
                {
                    ev_flags |= E_READ;
                }
                if(FD_ISSET(i, &wr_fd_set))
                {
                    ev_flags |= E_WRITE;
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
void evselect_reset(EVBASE *evbase)
{
    if(evbase)
    {
        if(evbase->ev_fds)
        {
            free(evbase->ev_fds);
            evbase->ev_fds = NULL;
        }
        if(evbase->ev_read_fds)
        {
            free(evbase->ev_read_fds);
            evbase->ev_read_fds = NULL;
        }
        if(evbase->ev_write_fds)
        {
            free(evbase->ev_write_fds);
            evbase->ev_write_fds = NULL;
        }
        if(evbase->evs)
        {
            free(evbase->evs);
            evbase->evs = NULL;
        }
        evbase->nfd = 0;
        evbase->maxfd = 0;
        evbase->nevent = 0;
        evbase->allowed = 0;
    }
}

/* Clean evbase */
void evselect_clean(EVBASE **evbase)
{
    if(*evbase)
    {
        if((*evbase)->logger)LOGGER_CLEAN((*evbase)->logger);
        if((*evbase)->evlist)free((*evbase)->evlist);
        if((*evbase)->evs)free((*evbase)->evs);
        if((*evbase)->ev_fds)free((*evbase)->ev_fds);
        if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
        if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
        free(*evbase);
        (*evbase) = NULL;
    }
}
#endif
