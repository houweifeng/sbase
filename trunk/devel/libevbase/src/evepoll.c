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
    return -1;
}

/* Add new event to evbase */
int evepoll_add(EVBASE *evbase, EVENT *event)
{
    int op = 0, ev_flags = 0, add = 0, ret = 0;
    struct epoll_event ep_event = {0, {0}};

    if(evbase && event && event->ev_fd >= 0  && event->ev_fd < evbase->allowed)
    {
        MUTEX_LOCK(evbase->mutex);
        DEBUG_LOGGER(evbase->logger, "Ready for adding event %d on fd[%d]", event->ev_flags, event->ev_fd);
        event->ev_base = evbase;
        /* Delete OLD garbage */
        //epoll_ctl(ebase->efd, EPOLL_CTL_DEL, event->ev_fd, NULL);
        memset(&(((struct epoll_event *)evbase->evs)[event->ev_fd]), 
                0, sizeof(struct epoll_event));
        if(event->ev_flags & E_READ)
        {
            ev_flags |= EPOLLIN;
            add = 1;
        }	
        if(event->ev_flags & E_WRITE)
        {
            ev_flags |= EPOLLOUT;
            add = 1;
        }
        if(add)
        {
            op = EPOLL_CTL_ADD; 

            //if(evbase->evlist[event->ev_fd]) op = EPOLL_CTL_MOD;
            ep_event.data.fd = event->ev_fd;
            ep_event.events = ev_flags;
            ep_event.data.ptr = (void *)event;
            if(epoll_ctl(evbase->efd, op, event->ev_fd, &ep_event) < 0)
            {
                FATAL_LOGGER(evbase->logger, "Added event %d op:%d on fd[%d] flags[%d] errno:%d {EBADF:%d, EEXIST:%d, EINVAL:%d, ENOENT:%d, ENOMEM:%d, EPERM:%d} failed, %s", ev_flags, op, event->ev_fd, event->ev_flags, errno, EBADF, EEXIST, EINVAL, ENOENT, ENOMEM, EPERM, strerror(errno));
                ret = -1;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "Added event %d on fd[%d]", ev_flags, event->ev_fd);
                if(event->ev_fd > evbase->maxfd)
                    evbase->maxfd = event->ev_fd;
                evbase->evlist[event->ev_fd] = event;
                ++(evbase->nfd);
            }
        }
        MUTEX_UNLOCK(evbase->mutex);
    }
    return ret;
}

/* Update event in evbase */
int evepoll_update(EVBASE *evbase, EVENT *event)
{
    struct epoll_event ep_event = {0, {0}};
    int op = 0, ev_flags = 0, mod = 0, ret = -1;

    if(evbase && event && event->ev_fd >= 0 && event->ev_fd < evbase->allowed)
    {
        MUTEX_LOCK(evbase->mutex);
        DEBUG_LOGGER(evbase->logger, "Ready for updating event %d on fd[%d]", event->ev_flags, event->ev_fd);
        if(event->ev_flags & E_READ)
        {
            ev_flags |= EPOLLIN;
            mod = 1;
        }
        if(event->ev_flags & E_WRITE)
        {
            ev_flags |= EPOLLOUT;
            mod = 1;
        }
        if(mod)
        {
            op = EPOLL_CTL_MOD;
            if(evbase->evlist[event->ev_fd] == NULL)
                op = EPOLL_CTL_ADD;
            ep_event.data.fd = event->ev_fd;
            ep_event.events = ev_flags;
            ep_event.data.ptr = (void *)event;
            if(epoll_ctl(evbase->efd, op, event->ev_fd, &ep_event) < 0)
            {
                FATAL_LOGGER(evbase->logger, "Update event %d op:%d on fd[%d] flags[%d] errno:%d {EBADF:%d, EEXIST:%d, EINVAL:%d, ENOENT:%d, ENOMEM:%d, EPERM:%d} failed, %s", ev_flags, op, event->ev_fd, event->ev_flags, errno, EBADF, EEXIST, EINVAL, ENOENT, ENOMEM, EPERM, strerror(errno));
                ret = -1;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "Updated event %d on fd[%d]", event->ev_flags, event->ev_fd);
                evbase->evlist[event->ev_fd] = event;
            }
        }
        MUTEX_UNLOCK(evbase->mutex);
        return 0;
    }
    return -1;	
}

/* Delete event from evbase */
int evepoll_del(EVBASE *evbase, EVENT *event)
{
    struct epoll_event ep_event = {0, {0}};

    if(evbase && event && event->ev_fd >= 0 && event->ev_fd < evbase->allowed)
    {
        MUTEX_LOCK(evbase->mutex);
        ep_event.data.fd = event->ev_fd;
        ep_event.events = EPOLLIN|EPOLLOUT;
        ep_event.data.ptr = (void *)event;
        epoll_ctl(evbase->efd, EPOLL_CTL_DEL, event->ev_fd, &ep_event);
        if(event->ev_fd >= evbase->maxfd)
            evbase->maxfd = event->ev_fd - 1;
        evbase->evlist[event->ev_fd] = NULL;
        if(evbase->nfd > 0) --(evbase->nfd);
        MUTEX_UNLOCK(evbase->mutex);
        return 0;
    }
    return -1;
}

/* Loop evbase */
int evepoll_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
    int i = 0, n = 0, timeout = 0, flags = 0, ev_flags = 0, fd = 0 ;
    struct epoll_event *evp = NULL;
    EVENT *ev = NULL;

    //if(evbase && evbase->nfd > 0)
    if(evbase)
    {
        if(tv) timeout = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
        //memset(evbase->evs, 0, sizeof(struct epoll_event) * evbase->maxfd);
        n = epoll_wait(evbase->efd, evbase->evs, evbase->allowed, timeout);
        //n = epoll_wait(evbase->efd, evbase->evs, evbase->maxfd+1, timeout);
        if(n == -1)
        {
            FATAL_LOGGER(evbase->logger, "Looping evbase[%p] error[%d] EBADF:%d EFAULT:%d EINTR:%d EINVAL:%d, %s", evbase, errno, EBADF, EFAULT, EINTR, EINVAL, strerror(errno));
        }
        if(n <= 0) return n;
        DEBUG_LOGGER(evbase->logger, "Actived %d event in %d", n, evbase->allowed);
        for(i = 0; i < n; i++)
        {
            evp = &(((struct epoll_event *)evbase->evs)[i]);
            ev = (EVENT *)evp->data.ptr;
            if(ev == NULL)
            {
                FATAL_LOGGER(evbase->logger, "Invalid i:%d evp:%p event:%p",
                        i, evp, ev);
                continue;
            }
            fd = ev->ev_fd;
            flags = evp->events;
            DEBUG_LOGGER(evbase->logger, "Activing i:%d evp:%p ev:%p fd:%d flags:%d",
                    i, evp, ev, fd, flags);
            //fd = evp->data.fd;
            if(fd >= 0 && fd < evbase->allowed &&  evbase->evlist[fd] 
                    && evp->data.ptr == (void *)evbase->evlist[fd])	
            {
                ev_flags = 0;
                if(flags & (EPOLLHUP|EPOLLERR))
                //if(flags & EPOLLERR)
                {
                    ev_flags |= E_READ;
                    ev_flags |= E_WRITE;
                }
                if(flags & EPOLLIN) ev_flags |= E_READ;
                if(flags & EPOLLOUT) ev_flags |= E_WRITE;
                if(ev_flags == 0) continue;
                DEBUG_LOGGER(evbase->logger, "Activing i:%d evp:%p ev:%p fd:%d ev_flags:%d", i, evp, ev, fd, ev_flags);
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
    return n;
}

/* Reset evbase */
void evepoll_reset(EVBASE *evbase)
{
    if(evbase)
    {
        close(evbase->efd);
        evbase->efd = epoll_create(evbase->allowed);
        evbase->nfd = 0;
        evbase->nevent = 0;
        evbase->maxfd = 0;
        memset(evbase->evs, 0, evbase->allowed * sizeof(struct epoll_event));
        memset(evbase->evlist, 0, evbase->allowed * sizeof(EVENT *));
        DEBUG_LOGGER(evbase->logger, "Reset evbase[%p]", evbase);
    }
    return ;
}

/* Clean evbase */
void evepoll_clean(EVBASE **evbase)
{
    if(*evbase)
    {
        close((*evbase)->efd);
        if((*evbase)->mutex){MUTEX_DESTROY((*evbase)->mutex);}
        if((*evbase)->logger)LOGGER_CLEAN((*evbase)->logger);
        if((*evbase)->evlist)free((*evbase)->evlist);
        if((*evbase)->evs)free((*evbase)->evs);
        if((*evbase)->ev_fds)free((*evbase)->ev_fds);
        if((*evbase)->ev_read_fds)free((*evbase)->ev_read_fds);
        if((*evbase)->ev_write_fds)free((*evbase)->ev_write_fds);
        if((*evbase)->efd > 0 )close((*evbase)->efd);
        free(*evbase);
        (*evbase) = NULL;
    }	
    return ;
}
#endif
