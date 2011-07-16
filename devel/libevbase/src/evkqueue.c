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
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#include "xmm.h"

/* Initialize evkqueue  */
int evkqueue_init(EVBASE *evbase)
{
    int max_fd = EV_MAX_FD;
    struct rlimit rlim;
    struct kevent event = {0}, kev = {0};

    if(evbase)
    {
        if((evbase->efd = kqueue()) == -1) 
            return -1;
        EV_SET(&kev, -1, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if(kevent(evbase->efd, &kev, 1, &event, 1, NULL) != 1
                || event.ident != -1 || event.flags != EV_ERROR)
        {
            close(evbase->efd);
            fprintf(stderr, "kevent test failed, %s\n", strerror(errno));
            return -1;
        }

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
    if(evbase && event && evbase->evs && event->ev_fd >= 0 && event->ev_fd < evbase->allowed)
    {
        event->ev_base = evbase;
        if(event->ev_flags & E_READ)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident  = event->ev_fd;
            kqev.filter = EVFILT_READ;
            kqev.flags  = EV_ADD;
            kqev.udata  = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) goto err;
            DEBUG_LOGGER(evbase->logger, "add EVFILT_READ[%d] on %d", 
                    (int)kqev.filter, (int)kqev.ident);
        }
        if(event->ev_flags & E_WRITE)
        {
            memset(&kqev, 0, sizeof(struct kevent));
            kqev.ident     = event->ev_fd;
            kqev.filter    = EVFILT_WRITE;
            kqev.flags     = EV_ADD;
            kqev.udata     = (void *)event;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1) goto err;
            DEBUG_LOGGER(evbase->logger, "add EVFILT_WRITE[%d] on %d", 
                    (int)kqev.filter, (int)kqev.ident);
        }
        evbase->evlist[event->ev_fd] = event;
        //SET_MAX_FD(evbase, event);
err:
        return 0;
    }
    return -1;
}

/* Update event in evbase */
int evkqueue_update(EVBASE *evbase, EVENT *event)
{
    int ev_flags = 0, ret = -1, add_ev_flags = 0, del_ev_flags = 0;
    struct kevent kqev;

    if(evbase && event && evbase->evs && event->ev_fd >= 0 
            && event->ev_fd < evbase->allowed)
    {
        ev_flags = (event->ev_flags ^ event->old_ev_flags);
        add_ev_flags = (event->ev_flags & ev_flags);
        del_ev_flags = (event->old_ev_flags & ev_flags);

        memset(&kqev, 0, sizeof(struct kevent));
        kqev.ident      = event->ev_fd;
        kqev.udata      = (void *)event;
        if(del_ev_flags & E_READ)
        {
            kqev.flags      = EV_DELETE;
            kqev.filter     = EVFILT_READ;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1)
            {   
                ERROR_LOGGER(evbase->logger, "deleting EVFILT_READ flags[%d] on fd[%d] failed, %s", 
                        kqev.flags, (int)kqev.ident, strerror(errno));
                goto err;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "deleted EVFILT_READ flags[%d] on %d",
                        kqev.flags, (int)kqev.ident);
                ret = 0;
            }
        }
        if(del_ev_flags & E_WRITE)
        {
            kqev.flags      = EV_DELETE;
            kqev.filter     = EVFILT_WRITE;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1)
            {   
                ERROR_LOGGER(evbase->logger, "deleting EVFILT_WRITE flags[%d] on %d failed, %s", 
                        kqev.flags, (int)kqev.ident, strerror(errno));
                goto err;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "deleted EVFILT_WRITE flags[%d] on %d",
                        kqev.flags, (int)kqev.ident);
                ret = 0;
            }
        }
        if(add_ev_flags & E_READ)
        {
            kqev.flags      = EV_ADD;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            kqev.filter     = EVFILT_READ;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1)
            {   
                ERROR_LOGGER(evbase->logger, "adding EVFILT_READ flags[%d] on fd[%d] failed, %s", 
                        kqev.flags, (int)kqev.ident, strerror(errno));
                goto err;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "added EVFILT_READ flags[%d] on fd[%d]",
                        kqev.flags, (int)kqev.ident);
                ret = 0;
            }
        }
        if(add_ev_flags & E_WRITE)
        {
            kqev.flags      = EV_ADD;
            if(!(event->ev_flags & E_PERSIST)) kqev.flags |= EV_ONESHOT;
            kqev.filter     = EVFILT_WRITE;
            if(kevent(evbase->efd, &kqev, 1, NULL, 0, NULL) == -1)
            {   
                ERROR_LOGGER(evbase->logger, "adding EVFILT_WRITE flags[%d] on fd[%d] failed, %s", 
                        kqev.flags, (int)kqev.ident, strerror(errno));
                goto err;
            }
            else
            {
                DEBUG_LOGGER(evbase->logger, "added EVFILT_WRITE flags[%d] on fd[%d]",
                        kqev.flags, (int)kqev.ident);
                ret = 0;
            }
        }
        //SET_MAX_FD(evbase, event);
        evbase->evlist[event->ev_fd] = event;
err:
        return ret;
    }
    return ret;
}

/* Delete event from evbase */
int evkqueue_del(EVBASE *evbase, EVENT *event)
{
    struct kevent kqev;
    if(evbase && event && evbase->evs 
            && event->ev_fd >= 0 && event->ev_fd < evbase->allowed)
    {
        memset(&kqev, 0, sizeof(struct kevent));
        kqev.ident  = event->ev_fd;
        kqev.filter = EVFILT_READ;
        kqev.flags  = EV_DELETE;
        kqev.udata  = (void *)event;
        kevent(evbase->efd, &kqev, 1, NULL, 0, NULL);
        DEBUG_LOGGER(evbase->logger, "del EVFILT_READ[%d] on %d", kqev.filter, (int)kqev.ident);
        kqev.filter = EVFILT_WRITE;
        kqev.flags  = EV_DELETE;
        kevent(evbase->efd, &kqev, 1, NULL, 0, NULL);
        DEBUG_LOGGER(evbase->logger, "del EVFILT_WRITE[%d] on %d", kqev.filter,(int)kqev.ident);
        evbase->evlist[event->ev_fd] = NULL;
        //RESET_MAX_FD(evbase, event);
        return 0;
    }
    return -1;
}
/* Loop evbase */
int evkqueue_loop(EVBASE *evbase, int loop_flags, struct timeval *tv)
{
    EVENT *ev = NULL;
    int i = 0, n = 0;
    int ev_flags = 0;	
    struct timespec ts = {0}, *pts = NULL;
    struct kevent *kqev = NULL;

    if(evbase)
    {
        if(tv) {TIMEVAL_TO_TIMESPEC(tv, &ts); pts = &ts;}
        n = kevent(evbase->efd, NULL, 0, (struct kevent *)evbase->evs, evbase->allowed, pts);	
        if(n == -1)
        {
            FATAL_LOGGER(evbase->logger, "Looping evbase[%p] error[%d], %s", 
                    evbase, errno, strerror(errno));
        }
        if(n <= 0 )return n;
        DEBUG_LOGGER(evbase->logger, "actived %d  in %d fd(s)", n, evbase->nfd);
        for(i = 0; i < n; i++)
        {
            kqev = &(((struct kevent *)evbase->evs)[i]);
            DEBUG_LOGGER(evbase->logger, "ident:%d fflags:%d EVFILT_READ:%d EVFILT_WRITE:%d", kqev->ident, kqev->filter, EVFILT_READ, EVFILT_WRITE);
            if(kqev && kqev->ident >= 0 && kqev->ident < evbase->allowed && evbase->evlist
                    && evbase->evlist[kqev->ident] == kqev->udata && (ev = kqev->udata))
            {
                ev_flags = 0;
                if(kqev->filter == EVFILT_READ)	ev_flags |= E_READ;
                else if(kqev->filter == EVFILT_WRITE) ev_flags |= E_WRITE;
                if(ev_flags == 0) continue;
                if(ev == evbase->evlist[kqev->ident] && (ev_flags &=  ev->ev_flags)) 
                {
                    event_active(ev, ev_flags);
                }
            }
        }
    }
    return n;
}

/* Reset evbase */
void evkqueue_reset(EVBASE *evbase)
{
    if(evbase)
    {
        close(evbase->efd);
        evbase->efd = kqueue();
        evbase->nfd = 0;
        evbase->nevent = 0;
        evbase->maxfd = 0;
        memset(evbase->evs, 0, evbase->allowed * sizeof(struct kevent));
        memset(evbase->evlist, 0, evbase->allowed * sizeof(EVENT *));
        DEBUG_LOGGER(evbase->logger, "Reset evbase[%p]", evbase);
    }
    return ;
}

/* Clean evbase */
void evkqueue_clean(EVBASE *evbase)
{
    if(evbase)
    {
        MUTEX_DESTROY(evbase->mutex);
        if(evbase->logger)LOGGER_CLEAN(evbase->logger);
        if(evbase->evlist)free(evbase->evlist);
        if(evbase->evs)free(evbase->evs);
        if(evbase->ev_fds)free(evbase->ev_fds);
        if(evbase->ev_read_fds)free(evbase->ev_read_fds);
        if(evbase->ev_write_fds)free(evbase->ev_write_fds);
        close(evbase->efd);
        xmm_free(evbase, sizeof(EVBASE));
    }	
    return ;
}
#endif
