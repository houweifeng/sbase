#include <errno.h>
#include "sbase.h"
#include "xssl.h"
#include "logger.h"
#include "service.h"
#include "mutex.h"
#include "memb.h"
#include "message.h"
#include "evtimer.h"
#ifndef UI
#define UI(_x_) ((unsigned int)(_x_))
#endif
#ifdef HAVE_SSL
#define SERVICE_CHECK_SSL_CLIENT(service)                                           \
do                                                                                  \
{                                                                                   \
    if(service->c_ctx == NULL)                                                      \
    {                                                                               \
        if((service->c_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)          \
        {                                                                           \
            ERR_print_errors_fp(stdout);                                            \
            _exit(-1);                                                              \
        }                                                                           \
    }                                                                               \
}while(0)
#else 
#define SERVICE_CHECK_SSL_CLIENT(service)
#endif
 
/* set service */
int service_set(SERVICE *service)
{
    char *p = NULL;
    int ret = -1, opt = 1;

    if(service)
    {
        p = service->ip;
        service->sa.sin_family = service->family;
        service->sa.sin_addr.s_addr = (p)? inet_addr(p):INADDR_ANY;
        service->sa.sin_port = htons(service->port);
        //service->connections = (CONN **)calloc(service->connections_limit, sizeof(CONN *));
        if(service->backlog <= 0) service->backlog = SB_CONN_MAX;
        SERVICE_CHECK_SSL_CLIENT(service);
        if(service->service_type == S_SERVICE)
        {
#ifdef HAVE_SSL
            if(service->is_use_SSL && service->cacert_file && service->privkey_file)
            {
                if((service->s_ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
                {
                    ERR_print_errors_fp(stdout);
                    _exit(-1);
                }
                /*load certificate */
                if(SSL_CTX_use_certificate_file(XSSL_CTX(service->s_ctx), service->cacert_file, 
                            SSL_FILETYPE_PEM) <= 0)
                {
                    ERR_print_errors_fp(stdout);
                    _exit(-1);
                }
                /*load private key file */
                if (SSL_CTX_use_PrivateKey_file(XSSL_CTX(service->s_ctx), service->privkey_file, 
                            SSL_FILETYPE_PEM) <= 0)
                {
                    ERR_print_errors_fp(stdout);
                    _exit(-1);
                }
                /*check private key file */
                if (!SSL_CTX_check_private_key(XSSL_CTX(service->s_ctx)))
                {
                    ERR_print_errors_fp(stdout);
                    exit(1);
                }
            }
#endif
            //flag = fcntl(service->fd, F_GETFL, 0);
            //flag |= O_NONBLOCK;
            //&& fcntl(service->fd, F_SETFL, flag) == 0
            if((service->fd = socket(service->family, service->sock_type, 0)) > 0 
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
#define PROCTHREAD_EXIT(id, exitid)                                                         
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
    int ret = -1, i = 0, x = 0;
    CONN *conn = NULL;

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
        //initliaze conns
        for(i = 0; i < SB_INIT_CONNS; i++)
        {
            if((conn = conn_init()))
            {
                x = service->nqconns++;
                service->qconns[x] = conn;
            }
            else break;
        }
        if(service->working_mode == WORKING_THREAD)
            goto running_threads;
        else 
            goto running_proc;
        return ret;
running_proc:
        //procthreads setting 
        if((service->daemon = procthread_init(0)))
        {
            PROCTHREAD_SET(service, service->daemon);
            if(service->daemon->message_queue)
            {
                if(service->daemon->message_queue) qmessage_clean(service->daemon->message_queue);
                service->daemon->message_queue = service->message_queue;
                service->daemon->ioqmessage = service->message_queue;
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
        if((service->daemon = procthread_init(0)))
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
        //iodaemon 
        if(service->use_iodaemon)
        {
            if((service->iodaemon = procthread_init(1)))
            {
                PROCTHREAD_SET(service, service->iodaemon);
                NEW_PROCTHREAD("iodaemon", 0, service->iodaemon->threadid, service->iodaemon, service->logger);
                ret = 0;
            }
            else
            {
                FATAL_LOGGER(service->logger, "Initialize new mode[%d] procthread failed, %s",
                        service->working_mode, strerror(errno));
                exit(EXIT_FAILURE);
                return -1;
            }
        }
        if(service->nprocthreads > SB_THREADS_MAX) service->nprocthreads = SB_THREADS_MAX;
        if(service->nprocthreads > 0 && (service->procthreads = (PROCTHREAD **)calloc(
                        service->nprocthreads, sizeof(PROCTHREAD *))))
        {
            for(i = 0; i < service->nprocthreads; i++)
            {
                if(service->use_iodaemon && service->iodaemon)
                {
                    if((service->procthreads[i] = procthread_init(0)))
                    {
                        PROCTHREAD_SET(service, service->procthreads[i]);
                        service->procthreads[i]->evbase = service->iodaemon->evbase;
                        service->procthreads[i]->ioqmessage = service->iodaemon->message_queue;
                        ret = 0;
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                        return -1;
                    }
                }
                else
                {
                    if((service->procthreads[i] = procthread_init(1)))
                    {
                        PROCTHREAD_SET(service, service->procthreads[i]);
                        service->procthreads[i]->ioqmessage = service->procthreads[i]->message_queue;
                        ret = 0;
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                        return -1;
                    }
                }
                NEW_PROCTHREAD("procthreads", i, service->procthreads[i]->threadid, 
                        service->procthreads[i], service->logger);
            }
        }
        if(service->ndaemons > SB_THREADS_MAX) service->ndaemons = SB_THREADS_MAX;
        if(service->ndaemons > 0 && (service->daemons = (PROCTHREAD **)calloc(
                        service->ndaemons, sizeof(PROCTHREAD *))))
        {
            for(i = 0; i < service->ndaemons; i++)
            {
                if((service->daemons[i] = procthread_init(0)))
                {
                    PROCTHREAD_SET(service, service->daemons[i]);
                    ret = 0;
                }
                else
                {
                    FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                            strerror(errno));
                    exit(EXIT_FAILURE);
                    return -1;
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
    char buf[SB_BUF_SIZE], *p = NULL, *ip = NULL;
    int fd = -1, port = -1, n = 0, opt = 1;
    SERVICE *service = (SERVICE *)arg;
    socklen_t rsa_len = sizeof(struct sockaddr_in);
    struct sockaddr_in rsa;
    CONN *conn = NULL;
#ifdef HAVE_SSL 
    SSL *ssl = NULL;
#endif

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
#ifdef HAVE_SSL
                        if(service->is_use_SSL && service->s_ctx)
                        {
                            if((ssl = SSL_new(XSSL_CTX(service->s_ctx))) && SSL_set_fd(ssl, fd) > 0 
                                    && SSL_accept(ssl) > 0)                                                   
                            {
                                goto new_conn;
                            }
                            else goto err_conn; 
                        }
#endif
new_conn:
                        if((conn = service_addconn(service, service->sock_type, fd, ip, port, 
                                        service->ip, service->port, &(service->session))))
                        {
#ifdef HAVE_SSL
                            conn->ssl = ssl;
#endif
                            DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d] via %d",
                                    ip, port, fd);
                            return ;
                        }else goto err_conn;
err_conn:               
#ifdef HAVE_SSL
                        if(ssl)
                        {
                            SSL_shutdown(ssl);
                            SSL_free(ssl);
                            ssl = NULL;
                        }
#endif
                        if(fd > 0)
                        {
                            shutdown(fd, SHUT_RDWR);
                            close(fd);
                        }
                        return ;
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
                            p = buf;
                            MB_PUSH(conn->buffer, p, n);
                            QMESSAGE_PUSH(((PROCTHREAD *)(conn->parent))->mutex, 
                                        ((PROCTHREAD *)(conn->parent))->message_queue, 
                                    MESSAGE_INPUT, -1, conn->fd, -1, conn, conn->parent, NULL);
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
#ifdef HAVE_SSL
    SSL *ssl = NULL;
#endif

    if(service && service->lock == 0)
    {
        family  = (inet_family > 0 ) ? inet_family : service->family;
        sock_type = (socket_type > 0 ) ? socket_type : service->sock_type;
        remote_ip = (inet_ip) ? inet_ip : service->ip;
        remote_port  = (inet_port > 0 ) ? inet_port : service->port;
        sess = (session) ? session : &(service->session);
        if((fd = socket(family, sock_type, 0)) > 0)
        {
            //DEBUG_LOGGER(service->logger, "new_conn[%s:%d] via %d", remote_ip, remote_port, fd);
            rsa.sin_family = family;
            rsa.sin_addr.s_addr = inet_addr(remote_ip);
            rsa.sin_port = htons(remote_port);
#ifdef HAVE_SSL
            if(sess->is_use_SSL &&  sock_type == SOCK_STREAM && service->c_ctx)
            {
                if((ssl = SSL_new(XSSL_CTX(service->c_ctx))) 
                        && connect(fd, (struct sockaddr *)&rsa, sizeof(rsa)) == 0 
                        && SSL_set_fd(ssl, fd) > 0 && SSL_connect(ssl) >= 0)
                {
                    goto new_conn;
                }
                else goto err_conn;
            }
#endif
            flag = fcntl(fd, F_GETFL, 0);
            flag |= O_NONBLOCK;
            if(fcntl(fd, F_SETFL, flag) == 0  && (connect(fd, (struct sockaddr *)&rsa, 
                            sizeof(rsa)) == 0 || errno == EINPROGRESS))
            {
                goto new_conn;
            }else goto err_conn;
new_conn:
            getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
            local_ip    = inet_ntoa(lsa.sin_addr);
            local_port  = ntohs(lsa.sin_port);
            if((conn = service_addconn(service, sock_type, fd, remote_ip, 
                            remote_port, local_ip, local_port, sess)))
            {
                if(flag != 0) conn->status = CONN_STATUS_READY; 
#ifdef HAVE_SSL
                conn->ssl = ssl;
#endif
                return conn;
            }else goto err_conn;
err_conn:
#ifdef HAVE_SSL
            if(ssl)
            {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = NULL;
            }
#endif
            if(fd > 0)
            {
                shutdown(fd, SHUT_RDWR);
                close(fd);
            }
            FATAL_LOGGER(service->logger, "connect to %s:%d via %d session[%p] failed, %s",
                    remote_ip, remote_port, fd, sess, strerror(errno));
            return conn;
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
#ifdef HAVE_SSL
    SSL *ssl = NULL;
#endif

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
#ifdef HAVE_SSL
            if(sess->is_use_SSL && sock_type == SOCK_STREAM && service->c_ctx)
            {
                DEBUG_LOGGER(service->logger, "SSL_newproxy() to %s:%d",remote_ip, remote_port);
                if((ssl = SSL_new(XSSL_CTX(service->c_ctx))) 
                        && SSL_set_fd(ssl, fd) > 0 && SSL_connect(ssl) >= 0)
                {
                    goto new_conn;
                }
                else goto err_conn;
            }
#endif
new_conn:
            getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
            local_ip    = inet_ntoa(lsa.sin_addr);
            local_port  = ntohs(lsa.sin_port);
            if((conn = service_addconn(service, sock_type, fd, remote_ip, remote_port, 
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
#ifdef HAVE_SSL
                conn->ssl = ssl;
#endif
                return conn;
            }
err_conn:
#ifdef HAVE_SSL
            if(ssl)
            {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = NULL;
            }
#endif
            if(fd > 0)
            {
                shutdown(fd, SHUT_RDWR);
                close(fd);
            }
            return conn;
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
        //fprintf(stdout, "%s::%d OK\n", __FILE__, __LINE__);
        if((conn = service_popfromq(service)))
        {
            conn->reset(conn);
        }
        else 
        {
            conn = conn_init();
            //fprintf(stdout, "newconn:%p\n", conn);
        }
        if(conn)
        {
            //fprintf(stdout, "%s::%d OK\n", __FILE__, __LINE__);
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
                    conn->parent    = service->daemon;
                    conn->ioqmessage = service->daemon->message_queue;
                    conn->message_queue = service->daemon->message_queue;
                    service->daemon->addconn(service->daemon, conn);
                }
                else
                {
                    //conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] on %s:%d "
                            "via %d  to service[%s]", remote_ip, remote_port, 
                            local_ip, local_port, fd, service->service_name);
                    service_pushtoq(service, conn);
                }
            }
            else if(service->working_mode == WORKING_THREAD)
            {
                index = fd % service->nprocthreads;
                if(service->procthreads && (procthread = service->procthreads[index]) 
                        && procthread->addconn)
                {
                    conn->parent = procthread;
                    conn->ioqmessage = procthread->ioqmessage;
                    conn->message_queue = procthread->message_queue;
                    procthread->addconn(procthread, conn);   
                    DEBUG_LOGGER(service->logger, "adding connection[%s:%d] via %d", conn->remote_ip, conn->remote_port, conn->fd);
                }
                else
                {
                    //conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] on %s:%d "
                            "via %d  to service[%s]", remote_ip, remote_port, 
                            local_ip, local_port, fd, service->service_name);
                    service_pushtoq(service, conn);
                }
            }
        }
    }
    return conn;
}

/* push connection to connections pool */
int service_pushconn(SERVICE *service, CONN *conn)
{
    int ret = -1, x = 0, id = 0, i = 0;
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
                if((x = conn->groupid) >= 0 && conn->groupid < SB_GROUPS_MAX)
                {
                    id = (service->groups[x].nconns_free)++;
                    service->groups[x].conns_free[id] = i;
                    conn->gindex = id;
                }
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
    int ret = -1, id = 0, x = 0;

    if(service && service->connections && conn)
    {
        MUTEX_LOCK(service->mutex);
        if(conn->index >= 0 && conn->index <= service->index_max
                && service->connections[conn->index] == conn)
        {
            if((x = conn->groupid) >= 0 && x < SB_GROUPS_MAX)
            {
                if((id = conn->gindex) >= 0 && id < SB_GROUPS_MAX)
                {
                    service->groups[x].conns_free[id] = -1;
                }
                --(service->groups[x].total);
            }
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
        else
        {
            FATAL_LOGGER(service->logger, "Removed connection[%s:%d] on %s:%d via %d "
                    "index[%d] of total %d failed", conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd, 
                    conn->index, service->running_connections);
        }
        MUTEX_UNLOCK(service->mutex);
        //return service_pushtoq(service, conn);
    }
    return ret;
}

/* get connection with free state */
CONN *service_getconn(SERVICE *service, int groupid)
{
    CONN *conn = NULL;
    int i = 0, x = 0;

    if(service)
    {
        MUTEX_LOCK(service->mutex);
        if(groupid >= 0 && groupid < service->ngroups)
        {
            while(service->groups[groupid].nconns_free > 0)
            {
                x = --(service->groups[groupid].nconns_free);
                if((i = service->groups[groupid].conns_free[x]) >= 0 && i < SB_CONN_MAX 
                        && (conn = service->connections[i]))
                {
                    conn->start_cstate(conn);
                    break;
                }
            }
        }
        else
        {
            while(service->nconns_free > 0)
            {
                x = --(service->nconns_free);
                if((i = service->conns_free[x]) >= 0 && i < SB_CONN_MAX 
                        && (conn = service->connections[i]))
                {
                    conn->start_cstate(conn);
                    break;
                }
            }
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return conn;
}

/* freeconn */
int service_freeconn(SERVICE *service, CONN *conn)
{
    int id = 0, x = 0;

    if(service && conn)
    {
        MUTEX_LOCK(service->mutex);
        if((id = conn->groupid) >= 0 && conn->groupid < SB_GROUPS_MAX)
        {
            x = service->groups[id].nconns_free++;
            service->groups[id].conns_free[x] = conn->index; 
        }
        else
        {
            x = service->nconns_free++; 
            service->conns_free[x] = conn->index;
        }
        MUTEX_UNLOCK(service->mutex);
        return 0;
    }
    return -1;
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
    CHUNK *cp = NULL;
    int x = 0;

    if(service && service->qchunks)
    {
        MUTEX_LOCK(service->mutex);
        if(service->nqchunks > 0 && (x = --(service->nqchunks)) >= 0 
                && (cp = service->qchunks[x]))
        {
            service->qchunks[x] = NULL;
            DEBUG_LOGGER(service->logger, "chunk_pop(%p) bsize:%d total:%d", cp, CK_BSIZE(cp), service->nqchunks);
        }
        else
        {
            CK_INIT(cp);
            if(cp){DEBUG_LOGGER(service->logger, "chunk_new(%p) bsize:%d", cp, CK_BSIZE(cp));}
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return cp;
}

/* push to qconns */
int service_pushtoq(SERVICE *service, CONN *conn)
{
    int x = 0;

    if(service && conn)
    {
        MUTEX_LOCK(service->mutex);
        x = service->nqconns++;
        service->qconns[x] = conn;
        MUTEX_UNLOCK(service->mutex);
    }
    return x;
}

/* push to qconn */
CONN *service_popfromq(SERVICE *service)
{
    CONN *conn = NULL;
    int x = 0;

    if(service)
    {
        MUTEX_LOCK(service->mutex);
        if(service->nqconns > 0)
        {
            x = --(service->nqconns);
            conn = service->qconns[x];
            service->qconns[x] = NULL;
        }
        //fprintf(stdout, "nqconns:%d\n", service->nqconns);
        MUTEX_UNLOCK(service->mutex);
    }
    return conn;
}

/* push chunk to service  */
int service_pushchunk(SERVICE *service, CHUNK *cp)
{
    int ret = -1, x = 0;

    if(service && service->qchunks && cp)
    {
        MUTEX_LOCK(service->mutex);
        CK_RESET(cp);
        x = service->nqchunks++;
        service->qchunks[x] = cp;
        DEBUG_LOGGER(service->logger, "chunk_push(%p) bsize:%d total:%d", 
                cp, CK_BSIZE(cp), service->nqchunks);
        MUTEX_UNLOCK(service->mutex);
    }
    return ret;
}

/* new chunk */
CB_DATA *service_newchunk(SERVICE *service, int len)
{
    CB_DATA *chunk = NULL;
    CHUNK *cp = NULL;

    if(service)
    {
        if((cp = service_popchunk(service)))
        {
            CK_MEM(cp, len); 
            chunk = (CB_DATA *)cp;
        }
    }
    return chunk;
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
    struct ip_mreq mreq;
    int ret = -1;

    if(service && service->sock_type == SOCK_DGRAM && multicast_ip 
            && service->ip && service->fd > 0)
    {
        memset(&mreq, 0, sizeof(struct ip_mreq));
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
    struct ip_mreq mreq;
    int ret = -1;

    if(service && service->sock_type == SOCK_DGRAM && service->ip && service->fd)
    {
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
        mreq.imr_interface.s_addr = inet_addr(service->ip);
        ret = setsockopt(service->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,(char*)&mreq, sizeof(mreq));
    }
    return ret;
}

/* broadcast */
int service_broadcast(SERVICE *service, char *data, int len)
{
    int ret = -1, i = 0;
    CONN *conn = NULL;

    if(service && service->running_connections > 0)
    {
        MUTEX_LOCK(service->mutex);
        for(i = 0; i < service->index_max; i++)
        {
            if((conn = service->connections[i]))
            {
                conn->push_chunk(conn, data, len);
            }
        }
        MUTEX_UNLOCK(service->mutex);
        ret = 0;
    }
    return ret;
}

/* add group */
int service_addgroup(SERVICE *service, char *ip, int port, int limit, SESSION *session)
{
    int id = -1;
    if(service && service->ngroups < SB_GROUPS_MAX)
    {
        MUTEX_LOCK(service->mutex);
        id = service->ngroups++;
        strcpy(service->groups[id].ip, ip);
        service->groups[id].port = port;
        service->groups[id].limit = limit;
        memcpy(&(service->groups[id].session), session, sizeof(SESSION));
        //fprintf(stdout, "%s::%d service[%s]->group[%d]->session.data_handler:%p\n", __FILE__, __LINE__, service->service_name, id, service->groups[id].session.data_handler);
        MUTEX_UNLOCK(service->mutex);
    }
    return id;
}

/* add group */
int service_closegroup(SERVICE *service, int groupid)
{
    int id = -1;
    if(service && groupid < SB_GROUPS_MAX)
    {
        MUTEX_LOCK(service->mutex);
        service->groups[groupid].limit = 0;
        MUTEX_UNLOCK(service->mutex);
    }
    return id;
}

/* group cast */
int service_castgroup(SERVICE *service, char *data, int len)
{
    CONN *conn = NULL;
    int i = 0;

    if(service && data && len > 0 && service->ngroups > 0)
    {
        for(i = 0; i < service->ngroups; i++)
        {
            if((conn = service_getconn(service, i)))
            {
                conn->start_cstate(conn);
                conn->groupid = i;
                conn->push_chunk(conn, data, len);
            }
        }
        return 0;
    }
    return -1;
}

/* state groups */
int service_stategroup(SERVICE *service)
{
    CONN *conn = NULL;
    int i = 0;

    if(service && service->ngroups > 0)
    {
        for(i = 0; i < service->ngroups; i++)
        {
            while(service->groups[i].limit > 0 
                    && service->groups[i].total < service->groups[i].limit
                    && (conn = service_newconn(service, 0, 0, service->groups[i].ip,
                            service->groups[i].port, &(service->groups[i].session))))
            {
                //fprintf(stdout, "%s::%d service[%s]->group[%d]->session.data_handler:%p\n", __FILE__, __LINE__, service->service_name, i, service->groups[i].session.data_handler);
                //fprintf(stdout, "%s::%d newconn[%s:%d] via %d \n", __FILE__, __LINE__, service->groups[i].ip, service->groups[i].port, conn->fd);
                conn->groupid = i;
                service->groups[i].total++;
            }
        }
        return 0;
    }
    return -1;
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
    CONN *conn = NULL;
    int i = 0;

    if(service)
    {
        //DEBUG_LOGGER(service->logger, "ready for stop service:%s\n", service->service_name);
        //stop all connections 
        if(service->connections && service->running_connections > 0 && service->index_max >= 0)
        {
            //DEBUG_LOGGER(service->logger, "Ready for close connections[%d]",  service->index_max);
            for(i = 0; i <= service->index_max; i++)
            {
                if((conn = service->connections[i]))
                {
                    conn->close(conn);
                }
            }
        }
        //stop all procthreads
        //daemon
        if(service->daemon)
        {
            //DEBUG_LOGGER(service->logger, "Ready for stop daemon");
            service->daemon->stop(service->daemon);
            //DEBUG_LOGGER(service->logger, "Ready for joinning daemon thread");
            //PROCTHREAD_EXIT(service->daemon->threadid, thread_exit);
            //DEBUG_LOGGER(service->logger, "Joinning daemon thread");
            //DEBUG_LOGGER(service->logger, "over for stop daemon");
        }
        //iodaemon
        if(service->iodaemon)
        {
            //DEBUG_LOGGER(service->logger, "Ready for stop daemon");
            service->iodaemon->stop(service->iodaemon);
            //DEBUG_LOGGER(service->logger, "Ready for joinning daemon thread");
            //PROCTHREAD_EXIT(service->iodaemon->threadid, thread_exit);
            //DEBUG_LOGGER(service->logger, "Joinning daemon thread");
            //DEBUG_LOGGER(service->logger, "over for stop daemon");
        }
        //threads
        if(service->procthreads && service->nprocthreads > 0)
        {
            //DEBUG_LOGGER(service->logger, "Ready for stop procthreads");
            for(i = 0; i < service->nprocthreads; i++)
            {
                if(service->procthreads[i])
                {
                    service->procthreads[i]->stop(service->procthreads[i]);
                    //PROCTHREAD_EXIT(service->procthreads[i]->threadid, thread_exit);
                }
            }
        }
        //daemons
        if(service->daemons && service->ndaemons > 0)
        {
            //DEBUG_LOGGER(service->logger, "Ready for stop daemonss");
            for(i = 0; i < service->ndaemons; i++)
            {
                if(service->daemons[i])
                {
                    service->daemons[i]->stop(service->daemons[i]);
                    //PROCTHREAD_EXIT(service->daemons[i]->threadid, thread_exit);
                }
            }
        }
        EVTIMER_DEL(service->evtimer, service->evid);
        //remove event
        if(service->event)service->event->destroy(service->event);
        service->lock = 1;
        close(service->fd);
    }
    return ;
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
            if(service->ngroups > 0)service_stategroup(service);
            else
            {
                if(service->running_connections < service->client_connections_limit)
                {
                    //DEBUG_LOGGER(service->logger, "Ready for state connection[%s:%d][%d] running:%d ",
                        //service->ip, service->port, service->client_connections_limit,
                        // service->running_connections);
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
    return ;
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
        
        //if((*pservice)->connections) free((*pservice)->connections);
        if((*pservice)->event) (*pservice)->event->clean(&((*pservice)->event)); 
        if((*pservice)->daemon) (*pservice)->daemon->clean(&((*pservice)->daemon));
        if((*pservice)->iodaemon) (*pservice)->iodaemon->clean(&((*pservice)->iodaemon));
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
        //DEBUG_LOGGER((*pservice)->logger, "Ready for clean connection_chunk");
        if((*pservice)->nqconns > 0)
        {
            //fprintf(stdout, "nqconns:%d\n", (*pservice)->nqconns);
            //DEBUG_LOGGER((*pservice)->logger, "Ready for clean connections");
            while((i = --((*pservice)->nqconns)) >= 0)
            {
                if((conn = ((*pservice)->qconns[i]))) 
                {
                    DEBUG_LOGGER((*pservice)->logger, "Ready for clean conn[%p]", conn);
                    conn->clean(&conn);
                }
            }
        }
        //clean connections
        /*
        for(i = 0; i < SB_CONN_MAX; i++)
        {
            if((conn = (*pservice)->connections[i]))
            {
                conn->clean(&conn);
            }
        }*/
        //clean chunks queue
        //DEBUG_LOGGER((*pservice)->logger, "Ready for clean chunks_queue");
        if((*pservice)->nqchunks > 0)
        {
            //DEBUG_LOGGER((*pservice)->logger, "Ready for clean chunks");
            while((i = --((*pservice)->nqchunks)) >= 0)
            {
                if((cp = (*pservice)->qchunks[i]))
                {
                    DEBUG_LOGGER((*pservice)->logger, "Ready for clean conn[%p]", cp);
                    CK_CLEAN(cp);
                }
            }
        }
        /* SSL */
#ifdef HAVE_SSL
        if((*pservice)->s_ctx) SSL_CTX_free(XSSL_CTX((*pservice)->s_ctx));
        if((*pservice)->c_ctx) SSL_CTX_free(XSSL_CTX((*pservice)->c_ctx));
#endif
        MUTEX_DESTROY((*pservice)->mutex);
        if((*pservice)->is_inside_logger) 
        {
            LOGGER_CLEAN((*pservice)->logger);
        }
        free(*pservice);
        *pservice = NULL;
    }
    return ;
}

/* Initialize service */
SERVICE *service_init()
{
    SERVICE *service = NULL;
    if((service = (SERVICE *)calloc(1, sizeof(SERVICE))))
    {
        MUTEX_INIT(service->mutex);
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
        service->freeconn           = service_freeconn;
        service->findconn           = service_findconn;
        service->popchunk           = service_popchunk;
        service->pushchunk          = service_pushchunk;
        service->newchunk           = service_newchunk;
        service->set_session        = service_set_session;
        service->add_multicast      = service_add_multicast;
        service->drop_multicast     = service_drop_multicast;
        service->broadcast          = service_broadcast;
        service->addgroup           = service_addgroup;
        service->closegroup         = service_closegroup;
        service->castgroup          = service_castgroup;
        service->stategroup         = service_stategroup;
        service->newtask            = service_newtask;
        service->newtransaction     = service_newtransaction;
        service->set_heartbeat      = service_set_heartbeat;
        service->clean              = service_clean;
    }
    return service;
}
