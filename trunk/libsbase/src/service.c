#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "service.h"
#include "queue.h"

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
        if(service->backlog <= 0) service->backlog = SB_CONN_MAX;
        if(service->service_type == S_SERVICE)
        {
            if((service->fd = socket(service->family, service->sock_type, 0)) > 0 
                && fcntl(service->fd, F_SETFL, O_NONBLOCK) == 0
                && setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0
                && bind(service->fd, (struct sockaddr *)&(service->sa), sizeof(service->sa)) == 0
                && listen(service->fd, service->backlog) == 0 
                && service->evbase && (service->event = ev_init()))
            {
                service->event->set(service->event, service->fd, E_READ|E_PERSIST,
                        (void *)service, (void *)&service_event_handler);
                ret = service->evbase->add(service->evbase, service->event);
            }
        
        }else if(service->service_type == C_SERVICE)
        {
            ret = 0;
        }
    }
    return ret;
}

#ifdef HAVE_PTHREAD
#define NEW_PROCTHREAD(ns, id, pthid, pth, logger)                                          \
{                                                                                           \
    if(pthread_create(&pthid, NULL, (void *)(pth->run), (void *)pth) == 0)                  \
    {                                                                                       \
        DEBUG_LOGGER(logger, "Created %s[%d] ID[%08x]", ns, id, pthid);                     \
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

/* running */
int service_run(SERVICE *service)
{
    int ret = -1;
    int i = 0;

    if(service)
    {
        if(service->working_mode == WORKING_PROC)
        {
            if((service->daemon = procthread_init()))
            {
                service->daemon->evbase = service->evbase;
                service->daemon->logger = service->logger;
                if(service->daemon->message_queue)
                {
                    QUEUE_CLEAN(service->daemon->message_queue);
                    service->daemon->message_queue = service->message_queue;
                }
                service->daemon->service = service;
                ret = 0;
            }
            else
            {
                FATAL_LOGGER(service->logger, "Initialize new mode[%d] procthread failed, %s",
                        service->working_mode, strerror(errno));
            }
        }
        else if(service->working_mode == WORKING_THREAD)
        {
            if(service->nprocthreads > 0 && (service->procthreads = (PROCTHREAD **)calloc(
                            service->nprocthreads, sizeof(PROCTHREAD *))))
            {
                for(i = 0; i < service->nprocthreads; i++)
                {
                    if((service->procthreads[i] = procthread_init()))
                    {
                        service->procthreads[i]->service = service;
                        service->procthreads[i]->logger = service->logger;
                        service->procthreads[i]->evbase = service->evbase;
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    NEW_PROCTHREAD("procthreads", i, procthread_id, 
                            service->procthreads[i], service->logger);
                }
            }
            if(service->ndaemons > 0 && (service->daemons = (PROCTHREAD **)calloc(
                            service->ndaemons, sizeof(PROCTHREAD *))))
            {
                for(i = 0; i < service->ndaemons; i++)
                {
                    if((service->daemons[i] = procthread_init()))
                    {
                        service->daemons[i]->service = service;
                        service->daemons[i]->logger = service->logger;
                        service->daemons[i]->evbase = service->evbase;
                    }
                    else
                    {
                        FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                                strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    NEW_PROCTHREAD("daemons", i, procthread_id, 
                            service->daemons[i], service->logger);
                }
            }
        }
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
    CONN *conn = NULL;
    int fd = -1;
    char *ip = NULL;
    int port = -1;

    if(service)
    {
        if(event_fd == service->fd)
        {
            if(E_READ & flag)
            {
                if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0)
                {
                    ip = inet_ntoa(rsa.sin_addr);
                    port = ntohs(rsa.sin_port);
                    if((conn = service_addconn(service, fd, ip, port, &(service->session))))
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
        }
    }
    return ;
}

/* new connection */
CONN *service_newconn(SERVICE *service, int inet_family, int socket_type, 
                char *inet_ip, int inet_port, SESSION *session)
{
    CONN *conn = NULL;
    struct sockaddr_in rsa;
    int fd = -1, family = -1, sock_type = -1, port = -1;
    char *ip = NULL;
    SESSION *sess = NULL;

    if(service)
    {
        family  = (inet_family > 0 ) ? inet_family : service->family;
        sock_type = (socket_type > 0 ) ? socket_type : service->sock_type;
        ip = (inet_ip) ? inet_ip : service->ip;
        port  = (inet_port > 0 ) ? inet_port : service->port;
        sess = (session) ? session : &(service->session);
        if((fd = socket(family, sock_type, 0)) > 0)
        {
            rsa.sin_family = family;
            rsa.sin_addr.s_addr = inet_addr(ip);
            rsa.sin_port = htons(port);
            if(fcntl(fd, F_SETFL, O_NONBLOCK) == 0 
                    && (connect(fd, (struct sockaddr *)&rsa, sizeof(rsa)) == 0 
                        || errno == EINPROGRESS))
            {
                conn = service->addconn(service, fd, ip, port, session);
            }
            else
            {
                FATAL_LOGGER(service->logger, "connect to %s:%d via %d session[%08x] failed, %s",
                        ip, port, fd, sess, strerror(errno));
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

/* add new connection */
CONN *service_addconn(SERVICE *service, int fd, char *ip, int port, SESSION *session)
{
    PROCTHREAD *procthread = NULL;
    CONN *conn = NULL;
    int index = 0;

    if(service && fd > 0 && ip && port > 0 && session)
    {
        if((conn = conn_init(fd, ip, port)))
        {
            conn->logger    = service->logger;
            conn->evbase    = service->evbase;
            conn->set_session(conn, session);
            /* add  to procthread */
            if(service->working_mode == WORKING_PROC)
            {
                if(service->daemon && service->daemon->addconn)
                {
                    service->daemon->addconn(service->daemon, conn);
                }
                else
                {
                    conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] "
                            "via %d  to service[%s]", ip, port, fd, service->service_name);
                }
            }
            else if(service->working_mode == WORKING_THREAD)
            {
                index = fd % service->nprocthreads;
                if(service->procthreads && (procthread = service->procthreads[index]))
                {
                    procthread->addconn(procthread, conn);   
                }
                else
                {
                    conn->clean(&conn);
                    FATAL_LOGGER(service->logger, "can not add connection[%s:%d] "
                            "via %d  to service[%s]", ip, port, fd, service->service_name);
                }
            }
        }
    }
    return conn;
}

/* push connection to connections pool */
int service_pushconn(SERVICE *service, CONN *conn)
{
}

/* pop connection from connections pool with index */
int service_popconn(SERVICE *service, int index)
{
}

/* get connection with free state */
CONN *service_getconn(SERVICE *service)
{
}

/* stop service */
int service_stop(SERVICE *service)
{
    if(service)
    {
    }
    return -1;
}

/* service clean */
void service_clean(SERVICE **pservice)
{
    if(*pservice)
    {
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
        service->set        = service_set;
        service->run        = service_run;
        service->set_log    = service_set_log;
        service->stop       = service_stop;
        service->clean      = service_clean;
    }
    return service;
}
