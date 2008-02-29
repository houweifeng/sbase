#include "evkqueue.h"
#include "logger.h"
#include <errno.h>
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
		if((evbase->evs		= calloc(max_fd, sizeof(struct kevent))) == NULL) 
            return -1;
        if((evbase->evlist  = (EVENT **)calloc(max_fd, sizeof(EVENT *))) == NULL)
            return -1;
		evbase->allowed = max_fd;
		return 0;
	}
	return -1;
}

/* Add new event to evbase */
int evkqueue_add(EVBASE *evbase, EVENT *event)
{
    struct kevent kqev;
    if(evbase && event && evbase->evs && event->ev_fd >= 0)
    {
        event->ev_base = evbase;
        if(event->ev_flags & E_READ)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident  = event->ev_fd;
            kqev.filter = EVFILT_READ;
            kqev.flags  = EV_ADD|EV_CLEAR;	
            kqev.udata  = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) return -1;
            DEBUG_LOGGER(evbase->logger, "add EVFILT_READ[%d] on %d", kqev.filter, kqev.ident);
        }
        if(event->ev_flags & E_WRITE)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident     = event->ev_fd;
            kqev.filter    = EVFILT_WRITE;
            kqev.flags     = EV_ADD|EV_CLEAR;
            kqev.udata     = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) return -1;
            DEBUG_LOGGER(evbase->logger, "add EVFILT_WRITE[%d] on %d", kqev.filter, kqev.ident);
        }
        if(event->ev_fd > evbase->maxfd)
            evbase->maxfd = event->ev_fd;
        evbase->evlist[event->ev_fd] = event;
        evbase->nfd++;
        return 0;
    }
    return -1;
}

/* Update event in evbase */
int evkqueue_update(EVBASE *evbase, EVENT *event)
{
    struct kevent kqev;
    if(evbase && event && evbase->evs 
            && event->ev_fd >= 0 && event->ev_fd <= evbase->maxfd)
    {
        if(event->ev_flags & E_READ)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident      = event->ev_fd;
            kqev.filter     = EVFILT_READ;
            kqev.flags      = EV_ADD|EV_CLEAR;
            kqev.udata      = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) return -1;
            DEBUG_LOGGER(evbase->logger, "update EVFILT_READ[%d] on %d", kqev.filter, kqev.ident);
        }
        if(event->ev_flags & E_WRITE)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident      = event->ev_fd;
            kqev.filter     = EVFILT_WRITE;
            kqev.flags      = EV_ADD|EV_CLEAR;
            kqev.udata      = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) return -1;
            DEBUG_LOGGER(evbase->logger, "update EVFILT_WRITE[%d] on %d", kqev.filter, kqev.ident);
        }
        evbase->evlist[event->ev_fd] = event;
        return 0;
    }
    return -1;
}

/* Delete event from evbase */
int evkqueue_del(EVBASE *evbase, EVENT *event)
{
    struct kevent kqev;
    if(evbase && event && evbase->evs 
            && event->ev_fd >= 0 && event->ev_fd <= evbase->maxfd)
    {
        if(event->ev_flags & E_READ)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident  = event->ev_fd;
            kqev.filter = EVFILT_READ;
            kqev.flags  = EV_DELETE;
            kqev.udata  = (void *)event;
            kevent(evbase->efd, &kqev, 1, NULL, 0, NULL);
            DEBUG_LOGGER(evbase->logger, "del EVFILT_READ[%d] on %d", kqev.filter, kqev.ident);
        }
        if(event->ev_flags & E_WRITE)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident      = event->ev_fd;
            kqev.filter     = EVFILT_WRITE;
            kqev.flags      = EV_DELETE;
            kqev.udata      = (void *)event;
            kevent(evbase->efd, &kqev, 1, NULL, 0, NULL);
            DEBUG_LOGGER(evbase->logger, "del EVFILT_WRITE[%d] on %d", kqev.filter, kqev.ident);
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

    if(evbase && evbase->nfd > 0)
    {
        memset(&ts, 0, sizeof(struct timespec));
        if(tv) TIMEVAL_TO_TIMESPEC(tv, &ts);
        n = kevent(evbase->efd, NULL, 0, (struct kevent *)evbase->evs, evbase->allowed, &ts);	
        if(n == -1)
        {
            FATAL_LOGGER(evbase->logger, "Looping evbase[%08x] error[%d], %s", 
                    evbase, errno, strerror(errno));
        }
        //DEBUG_LOGGER(evbase->logger, "n:%d fd:%d", n, evbase->nfd);
        for(i = 0; i < n; i++)
        {
            kqev = &(((struct kevent *)evbase->evs)[i]);
            //DEBUG_LOGGER(evbase->logger, "ident:%d fflags:%d", kqev->ident, kqev->filter);
            if(kqev && kqev->ident >= 0 && kqev->ident <= evbase->maxfd && evbase->evlist
                    && evbase->evlist[kqev->ident] == kqev->udata && kqev->udata)
            {
                if(kqev->filter & EVFILT_READ)	ev_flags |= E_READ;
                if(kqev->filter & EVFILT_WRITE)	ev_flags |= E_WRITE;
                if((ev_flags &=  evbase->evlist[kqev->ident]->ev_flags )!= 0) 
                {
                    evbase->evlist[kqev->ident]->active(evbase->evlist[kqev->ident], ev_flags);
                }
            }

        }
    }
}

/* Reset evbase */
void evkqueue_reset(EVBASE *evbase)
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
        close(evbase->efd);
    }
}

/* Clean evbase */
void evkqueue_clean(EVBASE **evbase)
{
    if(*evbase)
    {
        if((*evbase)->logger)CLOSE_LOGGER((*evbase)->logger);
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
