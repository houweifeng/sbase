#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "service.h"
#include "queue.h"
#include "mutex.h"
#include "memb.h"
#include "message.h"
#include "evtimer.h"
#ifndef UI
#define UI(_x_) ((unsigned int)(_x_))
#endif
/* set service */
int service_set(SERVICE *service)
{
    char *p = NULL;
    int ret = -1, opt = 1, flag = 0;

    if(service)
    {
        p = service->ip;
        service->sa.sin_family = service->family;
        service->sa.sin_addr.s_addr = (p)? inet_addr(p):INADDR_ANY;
        service->sa.sin_port = htons(service->port);
        service->connections = (CONN **)calloc(service->connections_limit, sizeof(CONN *));
        if(service->backlog <= 0) service->backlog = SB_CONN_MAX;
        if(service->service_type == S_SERVICE)
        {
            flag = fcntl(service->fd, F_GETFL, 0);
            flag |= O_NONBLOCK;
            if((service->fd = socket(service->family, service->sock_type, 0)) > 0 
                    && fcntl(service->fd, F_SETFL, flag) == 0
                    && setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0
#ifdef SO_REUSEPORT
                    && setsockopt(service->fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == 0
#endif
                    && (ret = bind(service->fd, (struct sockaddr *)&(service->sa), 
                        sizeof(service->sa))) == 0)
            {
                if(service->sock_type == SOCK_STREAM)
                    ret = listen(service->fd, service->backlog);
            }

        }
        else if(service->service_type == C_SERVICE)
        {
            ret = 0;
        }
    }
    return ret;
}

#ifdef HAVE_PTHREAD
#define NEW_PROCTHREAD(ns, id, pthid, pth, logger)                                          \
{                                                                                           \
    if(pthread_create((pthread_t *)&pthid, NULL, (void *)(pth->run), (void *)pth) == 0)     \
    {                                                                                       \
        DEBUG_LOGGER(logger, "Created %s[%d] ID[%p]", ns, id, (void*)((long)pthid));        \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        FATAL_LOGGER(logger, "Create %s[%d] failed, %s", ns, id, strerror(errno));          \
        exit(EXIT_FAILURE);                                                                 \
    }                                                                                       \
}
#else
#define NEW_PROCTHREAD(ns, id, pthid, pth, logger)
#endif
#ifdef HAVE_PTHREAD
#define PROCTHREAD_EXIT(id, exitid)                                                         \
{                                                                                           \
    pthread_join((pthread_t)id, (void **)&exitid);                                          \
    pthread_detach((pthread_t)id);                                                          \
}
#else
#define PROCTHREAD_EXIT(id, exitid)
#endif
#define PROCTHREAD_SET(service, pth)                                                        \
{                                                                                           \
    pth->service = service;                                                                 \
    pth->logger = service->logger;                                                          \
    pth->usec_sleep = service->usec_sleep;                                                  \
}

/* running */
int service_run(SERVICE *service)
{
    int ret = -1;
    int i = 0;

    if(service)
    {
        //added to evtimer 
        if(service->heartbeat_interval > 0)
        {
            EVTIMER_ADD(service->evtimer, service->heartbeat_interval, 
                    &service_evtimer_handler, (void *)service, service->evid);
            DEBUG_LOGGER(service->logger, "Added service[%s] to evtimer[%p][%d] interval:%d",
                    service->service_name, service->evtimer, service->evid, 
                    service->heartbeat_interval);
        }
        //evbase setting 
        if(service->service_type == S_SERVICE 
                && service->evbase && (service->event = ev_init()))
        {
            service->event->set(service->event, service->fd, E_READ|E_PERSIST,
                    (void *)service, (void *)&service_event_handler);
            ret = service->evbase->add(service->evbase, service->event);
        }
        if(service->working_mode == WORKING_THREAD)
            goto running_threads;
        else 
            goto running_proc;
        return ret;
running_proc:
        //procthreads setting 
        if((service->daemon = procthread_init()))
        {
            PROCTHREAD_SET(service, service->daemon);
            if(service->daemon->message_queue)
            {
                QUEUE_CLEAN(service->daemon->message_queue);
                service->daemon->message_queue = service->message_queue;
                service->daemon->evbase->clean(&(service->daemon->evbase));
                service->daemon->evbase = service->evbase;
            }
            DEBUG_LOGGER(service->logger, "sbase->q[%p] service->q[%p] daemon->q[%p]",
                    service->sbase->message_queue, service->message_queue, 
                    service->daemon->message_queue);
            service->daemon->service = service;
            ret = 0;
        }
        else
        {
            FATAL_LOGGER(service->logger, "Initialize new mode[%d] procthread failed, %s",
                    service->working_mode, strerror(errno));
        }
        return ret;
running_threads:
#ifdef HAVE_PTHREAD
        //daemon 
        if((service->daemon = procthread_init()))
        {
            PROCTHREAD_SET(service, service->daemon);
            NEW_PROCTHREAD("daemon", 0, service->daemon->threadid, service->daemon, service->logger);
            ret = 0;
        }
        else
        {
            FATAL_LOGGER(service->logger, "Initialize new mode[%d] procthread failed, %s",
                    service->working_mode, strerror(errno));
            exit(EXIT_FAILURE);
            return -1;
        }
        if(service->nprocthreads > SB_NDAEMONS_MAX) service->nprocthreads = SB_NDAEMONS_MAX;
        if(service->nprocthreads > 0 && (service->procthreads = (PROCTHREAD **)calloc(
                        service->nprocthreads, sizeof(PROCTHREAD *))))
        {
            for(i = 0; i < service->nprocthreads; i++)
            {
                if((service->procthreads[i] = procthread_init()))
                {
                    service->procthreads[i]->evbase = service->evbase;
                    PROCTHREAD_SET(service, service->procthreads[i]);
                }
                else
                {
                    FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                            strerror(errno));
                    exit(EXIT_FAILURE);
                }
                NEW_PROCTHREAD("procthreads", i, service->procthreads[i]->threadid, 
                        service->procthreads[i], service->logger);
            }
        }
        if(service->ndaemons > SB_NDAEMONS_MAX) service->ndaemons = SB_NDAEMONS_MAX;
        if(service->ndaemons > 0 && (service->daemons = (PROCTHREAD **)calloc(
                        service->ndaemons, sizeof(PROCTHREAD *))))
        {
            for(i = 0; i < service->ndaemons; i++)
            {
                if((service->daemons[i] = procthread_init()))
                {
                    PROCTHREAD_SET(service, service->daemons[i]);
                }
                else
                {
                    FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                            strerror(errno));
                    exit(EXIT_FAILURE);
                }
                NEW_PROCTHREAD("daemons", i, service->daemons[i]->threadid,
                        service->daemons[i], service->logger);
            }
        }
        return ret;
#else
        service->working_mode = WORKING_PROC;
        goto running_proc;
#endif
        return ret;
    }
    return ret;
}

/* set logfile  */
int service_set_log(SERVICE *service, char *logfile)
{
    if(service && logfile)
    {
        LOGGER_INIT(service->logger, logfile);
        DEBUG_LOGGER(service->logger, "Initialize logger %s", logfile);
        service->is_inside_logger = 1;
        return 0;
    }
    return -1;
}

/* event handler */
void service_event_handler(int event_fd, short flag, void *arg)
{
    struct sockaddr_in rsa;
    socklen_t rsa_len = sizeof(rsa);
    SERVICE *service = (SERVICE *)arg;
    int fd = -1, port = -1, n = 0, opt = 1;
    char buf[SB_BUF_SIZE], *ip = NULL;
    MESSAGE msg = {0};
    CONN *conn = NULL;

    if(service)
    {
        if(event_fd == service->fd)
        {
            if(E_READ & flag)
            {
                if(service->sock_type == SOCK_STREAM)
                {
                    if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0)
                    {
                        ip = inet_ntoa(rsa.sin_addr);
                        port = ntohs(rsa.sin_port);
                        if((conn = service_addconn(service, service->sock_type, fd, ip, port, 
                                        service->ip, service->port, &(service->session))))
                        {
                            DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d] via %d",
                                    ip, port, fd);
                        }
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Accept new connection failed, %s", 
                                strerror(errno));
                    }
                }
                else if(service->sock_type == SOCK_DGRAM)
                {
                    if((n = recvfrom(event_fd, buf, SB_BUF_SIZE, 
                            0, (struct sockaddr *)&rsa, &rsa_len)) > 0)
                    {
                        ip = inet_ntoa(rsa.sin_addr);
                        port = ntohs(rsa.sin_port);
                        if((fd = socket(AF_INET, SOCK_DGRAM, 0)) > 0 
                            && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0
#ifdef SO_REUSEPORT
                            && setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == 0
#endif
                            && bind(fd, (struct sockaddr *)&(service->sa), 
                                sizeof(struct sockaddr_in)) == 0
                            && connect(fd, (struct sockaddr *)&rsa, 
                                sizeof(struct sockaddr_in)) == 0
                            && (conn = service_addconn(service, service->sock_type, fd, 
                                ip, port, service->ip, service->port, &(service->session))))
                        {
                            MB_PUSH(conn->buffer, buf, n);
                            msg.fd          = conn->fd;
                            msg.msg_id      = MESSAGE_INPUT;
                            msg.index       = -1;
                            msg.parent      = conn->parent;
                            msg.handler     = conn;
                            QUEUE_PUSH(((PROCTHREAD *)(conn->parent))->message_queue, MESSAGE, &msg);
                            DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d] via %d"
                                    " buffer:%d", ip, port, fd, MB_NDATA(conn->buffer));
                        }
                        else
                        {
                            shutdown(fd, SHUT_RDWR);
                            close(fd);
                            FATAL_LOGGER(service->logger, "Accept new connection failed, %s", 
                                    strerror(errno));
                        }
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Accept new connection failed, %s", 
                                strerror(errno));
                    }

                }
            }
        }
    }
    return ;
}

/* new connection */
CONN *service_newconn(SERVICE *service, int inet_family, int socket_type, 
        char *inet_ip, int inet_port, SESSION *session)
{
    CONN *conn = NULL;
    struct sockaddr_in rsa, lsa;
    socklen_t lsa_len = sizeof(lsa);
    int fd = -1, family = -1, sock_type = -1, remote_port = -1, local_port = -1, flag = 0;
    char *local_ip = NULL, *remote_ip = NULL;
    SESSION *sess = NULL;

    if(service && service->lock == 0)
    {
        family  = (inet_family > 0 ) ? inet_family : service->family;
        sock_type = (socket_type > 0 ) ? socket_type : service->sock_type;
        remote_ip = (inet_ip) ? inet_ip : service->ip;
        remote_port  = (inet_port > 0 ) ? inet_port : service->port;
        sess = (session) ? session : &(service->session);
        if((fd = socket(family, sock_type, 0)) > 0)
        {
            rsa.sin_family = family;
            rsa.sin_addr.s_addr = inet_addr(remote_ip);
            rsa.sin_port = htons(remote_port);
            flag = fcntl(fd, F_GETFL, 0);
            flag |= O_NONBLOCK;
            if(fcntl(fd, F_SETFL, flag) == 0 
                    && (connect(fd, (struct sockaddr *)&rsa, sizeof(rsa)) == 0 
                        || errno == EINPROGRESS))
            {
                getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
                local_ip    = inet_ntoa(lsa.sin_addr);
                local_port  = ntohs(lsa.sin_port);
                if((conn = service->addconn(service, sock_type, fd, remote_ip, remote_port, 
                                local_ip, local_port, sess)))
                    conn->status = CONN_STATUS_READY; 
            }
            else
            {
                FATAL_LOGGER(service->logger, "connect to %s:%d via %d session[%p] failed, %s",
                        remote_ip, remote_port, fd, sess, strerror(errno));
            }
        }
        else
        {
            FATAL_LOGGER(service->logger, "socket(%d, %d, 0) failed, %s", 
                    family, sock_type, strerror(errno));
        }
    }
    return conn;
}

/* new connection */
CONN *service_newproxy(SERVICE *service, CONN *parent, int inet_family, int socket_type, 
        char *inet_ip, int inet_port, SESSION *session)
{
    CONN *conn = NULL;
    struct sockaddr_in rsa, lsa;
    socklen_t lsa_len = sizeof(lsa);
    int fd = -1, family = -1, sock_type = -1, remote_port = -1, local_port = -1;
    char *local_ip = NULL, *remote_ip = NULL;
    SESSION *sess = NULL;

    if(service && service->lock == 0 && parent)
    {
        if(parent && (conn = parent->session.child))
        {
            conn->session.parent = NULL;
            conn->over(conn);
            conn = NULL;
        }
        family  = (inet_family > 0 ) ? inet_family : service->family;
        sock_type = (socket_type > 0 ) ? socket_type : service->sock_type;
        remote_ip = (inet_ip) ? inet_ip : service->ip;
        remote_port  = (inet_port > 0 ) ? inet_port : service->port;
        sess = (session) ? session : &(service->session);
        rsa.sin_family = family;
        rsa.sin_addr.s_addr = inet_addr(remote_ip);
        rsa.sin_port = htons(remote_port);
        if((fd = socket(family, sock_type, 0)) > 0
                && connect(fd, (struct sockaddr *)&rsa, sizeof(rsa)) == 0)
        {
            getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
            local_ip    = inet_ntoa(lsa.sin_addr);
            local_port  = ntohs(lsa.sin_port);
            if((conn = service->addconn(service, sock_type, fd, remote_ip, remote_port, 
                            local_ip, local_port, sess)))
            {
                if(conn->session.timeout == 0)
                    conn->session.timeout = SB_PROXY_TIMEOUT;
                if(parent->session.timeout == 0)
                    parent->session.timeout = SB_PROXY_TIMEOUT;
                parent->session.packet_type |= PACKET_PROXY;
                conn->session.packet_type |= PACKET_PROXY;
                conn->session.parent = parent;
                conn->session.parentid = parent->index;
            }
        }
        else
        {
            FATAL_LOGGER(service->logger, "connect to %s:%d via %d session[%p] failed, %s",
                    remote_ip, remote_port, fd, sess, strerror(errno));
        }
    }
    return conn;
}


/* add new connection */
CONN *service_addconn(SERVICE *service, int sock_type, int fd, char *remote_ip, int remote_port, 
        char *local_ip, int local_port, SESSION *session)
{
    PROCTHREAD *procthread = NULL;
    CONN *conn = NULL;
    int index = 0;

    if(service && service->lock == 0 && fd > 0 && session)
    {
        QUEUE_POP(service->connection_queue, PCONN, &conn);
        if(conn) conn->reset(conn);
        else conn = conn_init();
        if(conn)
        {
            conn->fd = fd;
            strcpy(conn->remote_ip, remote_ip);
            conn->remote_port = remote_port;
            strcpy(conn->local_ip, local_ip);
            conn->local_port = local_port;
            conn->sock_type = sock_type;
            conn->evtimer   = service->evtimer;
            conn->logger    = service->logger;
            conn->set_session(conn, session);
            /* add  to procthread */
            if(service->working_mode == WORKING_PROC)
            {
                if(service->daemon && service->daemon->addconn)
                {
                    conn->parent = service->daemon;
                    service->daemon->addconn(service->daemon, conn);
                }
                else
                {
                    conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] on %s:%d "
                            "via %d  to service[%s]", remote_ip, remote_port, 
                            local_ip, local_port, fd, service->service_name);
                }
            }
            else if(service->working_mode == WORKING_THREAD)
            {
                index = fd % service->nprocthreads;
                if(service->procthreads && (procthread = service->procthreads[index]) 
                        && procthread->addconn)
                {
                    conn->parent = procthread;
                    procthread->addconn(procthread, conn);   
                }
                else
                {
                    conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] on %s:%d "
                            "via %d  to service[%s]", remote_ip, remote_port, 
                            local_ip, local_port, fd, service->service_name);
                }
            }
        }
    }
    return conn;
}

/* push connection to connections pool */
int service_pushconn(SERVICE *service, CONN *conn)
{
    int ret = -1, i = 0;
    CONN *parent = NULL;

    if(service && conn && service->connections)
    {
        MUTEX_LOCK(service->mutex);
        for(i = 0; i < service->connections_limit; i++)
        {
            if(service->connections[i] == NULL)
            {
                service->connections[i] = conn;
                conn->index = i;
                service->running_connections++;
                if(i >= service->index_max) service->index_max = i;
                ret = 0;
                DEBUG_LOGGER(service->logger, "Added new connection[%s:%d] on %s:%d via %d "
                        "index[%d] of total %d", conn->remote_ip, conn->remote_port, 
                        conn->local_ip, conn->local_port, conn->fd, 
                        conn->index, service->running_connections);
                break;
            }
        }
        //for proxy
        if((conn->session.packet_type & PACKET_PROXY)
                && (parent = (CONN *)(conn->session.parent)) 
                && conn->session.parentid  >= 0 
                && conn->session.parentid < service->index_max 
                && conn->session.parent == service->connections[conn->session.parentid])
        {
            parent->bind_proxy(parent, conn);
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return ret;
}

/* pop connection from connections pool with index */
int service_popconn(SERVICE *service, CONN *conn)
{
    int ret = -1;

    if(service && service->connections && conn)
    {
        MUTEX_LOCK(service->mutex);
        if(conn->index >= 0 && conn->index <= service->index_max
                && service->connections[conn->index] == conn)
        {
            service->connections[conn->index] = NULL;
            service->running_connections--;
            if(service->index_max == conn->index) 
                service->index_max--;
            DEBUG_LOGGER(service->logger, "Removed connection[%s:%d] on %s:%d via %d "
                    "index[%d] of total %d", conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd, 
                    conn->index, service->running_connections);
            ret = 0;
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return ret;
}

/* get connection with free state */
CONN *service_getconn(SERVICE *service)
{
    CONN *conn = NULL;
    int i = 0;

    if(service)
    {
        MUTEX_LOCK(service->mutex);
        for(i = 0; i <= service->index_max; i++)
        {
            if((conn = service->connections[i]) && conn->status == CONN_STATUS_FREE
                    && conn->c_state == C_STATE_FREE)
            {
                //DEBUG_LOGGER(service->logger, "Got connection[%s:%d] on %s:%d via %d", 
                //conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
                conn->start_cstate(conn);
                break;
            }
            conn = NULL;
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return conn;
}

/* find connection as index */
CONN *service_findconn(SERVICE *service, int index)
{
    CONN *conn = NULL;
    if(service && index >= 0 && index <= service->index_max)
    {
        conn = service->connections[index];
    }
    return conn;
}

/* pop chunk from service  */
CHUNK * service_popchunk(SERVICE *service)
{
    int ret = -1;
    CHUNK *cp = NULL;

    if(service && service->chunks_queue)
    {
        MUTEX_LOCK(service->mutex);
        QUEUE_POP(service->chunks_queue, PCHUNK, &cp);
        if(cp == NULL) 
        {
            CK_INIT(cp);
            if(cp){DEBUG_LOGGER(service->logger, "chunk_new(%p) bsize:%d", cp, CK_BSIZE(cp));}
        }
        else
        {
            DEBUG_LOGGER(service->logger, "chunk_pop(%p) bsize:%d", cp, CK_BSIZE(cp));

        }
        MUTEX_UNLOCK(service->mutex);
    }
    return cp;
}

/* push chunk to service  */
int service_pushchunk(SERVICE *service, CHUNK *cp)
{
    int ret = -1;

    if(service && service->chunks_queue && cp)
    {
        MUTEX_LOCK(service->mutex);
        CK_RESET(cp);
        QUEUE_PUSH(service->chunks_queue, PCHUNK, &cp);
        DEBUG_LOGGER(service->logger, "chunk_push(%p) bsize:%d", cp, CK_BSIZE(cp));
        MUTEX_UNLOCK(service->mutex);
    }
    return ret;
}


/* set service session */
int service_set_session(SERVICE *service, SESSION *session)
{
    if(service && session)
    {
        memcpy(&(service->session), session, sizeof(SESSION));
        return 0;
    }
    return -1;
}
/* add multicast */
int service_add_multicast(SERVICE *service, char *multicast_ip)
{
    struct ip_mreq mreq = {0};
    int ret = -1;
    if(service && service->sock_type == SOCK_DGRAM && multicast_ip 
            && service->ip && service->fd > 0)
    {
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = inet_addr(service->ip);
        if((ret = setsockopt(service->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                        (char*)&mreq, sizeof(struct ip_mreq))) == 0)
        {
            DEBUG_LOGGER(service->logger, "added multicast:%s to service[%p]->fd[%d]",
                    multicast_ip, service, service->fd);
        }
    }
    return ret;
}

/* drop multicast */
int service_drop_multicast(SERVICE *service, char *multicast_ip)
{
    struct ip_mreq mreq = {0};
    int ret = -1;

    if(service && service->sock_type == SOCK_DGRAM && service->ip && service->fd)
    {
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = inet_addr(service->ip);
        ret = setsockopt(service->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,(char*)&mreq, sizeof(mreq));
    }
    return ret;
}

/* new task */
int service_newtask(SERVICE *service, CALLBACK *task_handler, void *arg)
{
    PROCTHREAD *pth = NULL;
    int index = 0, ret = -1;

    if(service)
    {
        /* Add task for procthread */
        if(service->working_mode == WORKING_PROC && service->daemon)
            pth = service->daemon;
        else if(service->working_mode == WORKING_THREAD && service->daemons)
        {
            index = service->ntask % service->ndaemons;
            pth = service->daemons[index];
        }
        if(pth)
        {
            pth->newtask(pth, task_handler, arg);
            service->ntask++;
            ret = 0;
        }
    }
    return ret;
}

/* add new transaction */
int service_newtransaction(SERVICE *service, CONN *conn, int tid)
{
    PROCTHREAD *pth = NULL;
    int index = 0, ret = -1;

    if(service && conn && conn->fd > 0)
    {
        /* Add transaction for procthread */
        if(service->working_mode == WORKING_PROC && service->daemon)
        {
            DEBUG_LOGGER(service->logger, "Adding transaction[%d] to %s:%d on %s:%d "
                    "to procthread[%d]", tid, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, getpid());
            return service->daemon->newtransaction(service->daemon, conn, tid);
        }
        /* Add transaction to procthread pool */
        if(service->working_mode == WORKING_THREAD && service->procthreads)
        {
            index = conn->fd % service->nprocthreads;
            pth = service->procthreads[index];
            DEBUG_LOGGER(service->logger, "Adding transaction[%d] to %s:%d on %s:%d "
                    "to procthreads[%d]", tid, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port,index);
            if(pth && pth->newtransaction)
                return pth->newtransaction(pth, conn, tid);
        }
    }
    return ret;
}

/* stop service */
void service_stop(SERVICE *service)
{
    int i = 0, *thread_exit = NULL;
    CONN *conn = NULL;

    if(service)
    {
        //stop all connections 
        service->lock = 1;
        if(service->connections && service->running_connections > 0 && service->index_max >= 0)
        {
            DEBUG_LOGGER(service->logger, "Ready for close connections[%d]",  service->index_max);
            for(i = 0; i <= service->index_max; i++)
            {
                if((conn = service->connections[i]))
                {
                    conn->close(conn);
                }
            }
        }
        //stop all procthreads
        if(service->daemon)
        {
            DEBUG_LOGGER(service->logger, "Ready for stop daemon");
            service->daemon->stop(service->daemon);
            DEBUG_LOGGER(service->logger, "Ready for joinning daemon thread");
            PROCTHREAD_EXIT(service->daemon->threadid, thread_exit);
            DEBUG_LOGGER(service->logger, "Joinning daemon thread");
            DEBUG_LOGGER(service->logger, "over for stop daemon");
        }
        if(service->procthreads && service->nprocthreads > 0)
        {
            DEBUG_LOGGER(service->logger, "Ready for stop procthreads");
            for(i = 0; i < service->nprocthreads; i++)
            {
                if(service->procthreads[i])
                {
                    service->procthreads[i]->stop(service->procthreads[i]);
                    PROCTHREAD_EXIT(service->procthreads[i]->threadid, thread_exit);
                }
            }
        }
        if(service->daemons && service->ndaemons > 0)
        {
            DEBUG_LOGGER(service->logger, "Ready for stop daemonss");
            for(i = 0; i < service->ndaemons; i++)
            {
                if(service->daemons[i])
                {
                    service->daemons[i]->stop(service->daemons[i]);
                    PROCTHREAD_EXIT(service->daemons[i]->threadid, thread_exit);
                }
            }
        }
        EVTIMER_DEL(service->evtimer, service->evid);
        //remove event
        if(service->event)service->event->destroy(service->event);
        close(service->fd);
    }
}

/* state check */
void service_state(void *arg)
{
    SERVICE *service = (SERVICE *)arg;
    int n = 0;

    if(service)
    {
        if(service->service_type == C_SERVICE)
        {
            if(service->running_connections < service->client_connections_limit)
            {
                DEBUG_LOGGER(service->logger, "Ready for state connection[%s:%d][%d] running:%d ",
                        service->ip, service->port, service->client_connections_limit,
                        service->running_connections);
                n = service->client_connections_limit - service->running_connections;
                while(n > 0)
                {
                    if(service->newconn(service, -1, -1, NULL, -1, NULL) == NULL)
                    {
                        FATAL_LOGGER(service->logger, "connect to %s:%d failed, %s", 
                                service->ip, service->port, strerror(errno));
                            break;
                    }
                    n--;
                }
            }
        }
    }
}

/* heartbeat handler */
void service_set_heartbeat(SERVICE *service, int interval, CALLBACK *handler, void *arg)
{
    if(service)
    {
        service->heartbeat_interval = interval;
        service->heartbeat_handler = handler;
        service->heartbeat_arg = arg;
        if(service->evtimer)
        {
            EVTIMER_ADD(service->evtimer, service->heartbeat_interval, 
                    &service_evtimer_handler, (void *)service, service->evid);
        }
    }
    return ;
}

/* active heartbeat */
void service_active_heartbeat(void *arg)
{
    SERVICE *service = (SERVICE *)arg;
    LOGGER logger = {0};

    if(service)
    {
        service_state(service);
        if(service->heartbeat_handler)
        {
            service->heartbeat_handler(service->heartbeat_arg);
        }
        //if(service->evid == 0)fprintf(stdout, "Ready for updating evtimer[%p][%d] [%p][%p] count[%d] q[%d]\n", service->evtimer, service->evid, PEVT_EVN(service->evtimer, service->evid)->prev, PEVT_EVN(service->evtimer, service->evid)->next, PEVT_NLIST(service->evtimer), PEVT_NQ(service->evtimer));
        //if(service->evid == 0) EVTIMER_LIST(service->evtimer, stdout);
        EVTIMER_UPDATE(service->evtimer, service->evid, service->heartbeat_interval, 
                &service_evtimer_handler, (void *)service);
        //if(service->evid == 0)fprintf(stdout, "Over for updating evtimer[%p][%d] [%p][%p] count[%d] q[%d]\n", service->evtimer, service->evid, PEVT_EVN(service->evtimer, service->evid)->prev, PEVT_EVN(service->evtimer, service->evid)->next, PEVT_NLIST(service->evtimer), PEVT_NQ(service->evtimer));
        //if(service->evid == 0) EVTIMER_LIST(service->evtimer, stdout);
    }
    return ;
}

/* active evtimer heartbeat */
void service_evtimer_handler(void *arg)
{
    SERVICE *service = (SERVICE *)arg;

    if(service && service->daemon)
    {
        //DEBUG_LOGGER(service->logger, "Ready for activing evtimer[%p][%d] count[%d] q[%d]", service->evtimer, service->evid, PEVT_NLIST(service->evtimer), PEVT_NQ(service->evtimer));
        service->daemon->active_heartbeat(service->daemon, 
                &service_active_heartbeat, (void *)service);
        //DEBUG_LOGGER(service->logger, "Over for activing evtimer[%p][%d] count[%d] q[%d]", service->evtimer, service->evid, PEVT_NLIST(service->evtimer), PEVT_NQ(service->evtimer));
    }
    return ;
}

/* service clean */
void service_clean(SERVICE **pservice)
{
    CONN *conn = NULL;
    CHUNK *cp = NULL;
    int i = 0;

    if(pservice && *pservice)
    {
        
        if((*pservice)->connections) free((*pservice)->connections);
        if((*pservice)->event) (*pservice)->event->clean(&((*pservice)->event)); 
        if((*pservice)->daemon) (*pservice)->daemon->clean(&((*pservice)->daemon));
        
        //clean procthreads
        if((*pservice)->procthreads && (*pservice)->nprocthreads)
        {
            for(i = 0; i < (*pservice)->nprocthreads; i++)
            {
                if((*pservice)->procthreads[i])
                {
                    (*pservice)->procthreads[i]->clean(&((*pservice)->procthreads[i]));
                }
            }
            free((*pservice)->procthreads);
        }
        
        //clean daemons
        if((*pservice)->daemons && (*pservice)->ndaemons)
        {
            for(i = 0; i < (*pservice)->ndaemons; i++)
            {
                if((*pservice)->daemons[i])
                {
                    (*pservice)->daemons[i]->clean(&((*pservice)->daemons[i]));
                }
            }
            free((*pservice)->daemons);
        }
        //clean connection_queue
        DEBUG_LOGGER((*pservice)->logger, "Ready for clean connection_chunk");
        if((*pservice)->connection_queue)
        {
            DEBUG_LOGGER((*pservice)->logger, "Ready for clean connections");
            while(QTOTAL((*pservice)->connection_queue) > 0)
            {
                conn = NULL;
                QUEUE_POP((*pservice)->connection_queue, PCONN, &conn);
        
                DEBUG_LOGGER((*pservice)->logger, "Ready for clean conn[%p]", conn);
                if(conn) conn->clean(&conn);
            }
            QUEUE_CLEAN((*pservice)->connection_queue);
        }
        
        //clean chunks queue
        DEBUG_LOGGER((*pservice)->logger, "Ready for clean chunks_queue");
        
        if((*pservice)->chunks_queue)
        {
            DEBUG_LOGGER((*pservice)->logger, "Ready for clean chunks");
            while(QTOTAL((*pservice)->chunks_queue) > 0)
            {
                cp = NULL;
                QUEUE_POP((*pservice)->chunks_queue, PCONN, &cp);
                DEBUG_LOGGER((*pservice)->logger, "Ready for clean conn[%p]", cp);
                if(cp){CK_CLEAN(cp);}
            }
            QUEUE_CLEAN((*pservice)->chunks_queue);
        }
        
        MUTEX_DESTROY((*pservice)->mutex);
        if((*pservice)->is_inside_logger && (*pservice)->logger) 
        {
            LOGGER_CLEAN((*pservice)->logger);
        }
        free(*pservice);
        *pservice = NULL;
    }
}

/* Initialize service */
SERVICE *service_init()
{
    SERVICE *service = NULL;
    if((service = (SERVICE *)calloc(1, sizeof(SERVICE))))
    {
        MUTEX_INIT(service->mutex);
        QUEUE_INIT(service->connection_queue);
        QUEUE_INIT(service->chunks_queue);
        service->set                = service_set;
        service->run                = service_run;
        service->set_log            = service_set_log;
        service->stop               = service_stop;
        service->newproxy           = service_newproxy;
        service->newconn            = service_newconn;
        service->addconn            = service_addconn;
        service->pushconn           = service_pushconn;
        service->popconn            = service_popconn;
        service->getconn            = service_getconn;
        service->findconn           = service_findconn;
        service->popchunk           = service_popchunk;
        service->pushchunk          = service_pushchunk;
        service->set_session        = service_set_session;
        service->add_multicast      = service_add_multicast;
        service->drop_multicast     = service_drop_multicast;
        service->newtask            = service_newtask;
        service->newtransaction     = service_newtransaction;
        service->set_heartbeat      = service_set_heartbeat;
        service->clean              = service_clean;
    }
    return service;
}
