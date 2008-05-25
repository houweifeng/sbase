#include <evbase.h>
#include "sbase.h"
#include "service.h"
#include "procthread.h"
#include "logger.h"
#include "conn.h"
#include "message.h"

/* Initialize service */
SERVICE *service_init()
{
    SERVICE *service = (SERVICE *)calloc(1, sizeof(SERVICE));
    if(service)
    {
        service->event_handler	    = service_event_handler;
        service->set		        = service_set;
        service->run		        = service_run;
        service->newtask		    = service_newtask;
        service->newtransaction		= service_newtransaction;
        service->addconn	        = service_addconn;
        service->newconn            = service_newconn;
        service->getconn            = service_getconn;
        service->pushconn           = service_pushconn;
        service->popconn            = service_popconn;
        service->active_heartbeat    = service_active_heartbeat;
        service->state_conns        = service_state_conns;
        service->terminate	        = service_terminate;
        service->clean		        = service_clean;
        service->event 		        = ev_init();
        TIMER_INIT(service->timer);
        MUTEX_INIT(service->mutex);
    }
    return service;
}

/* Set service  */
int service_set(SERVICE *service)
{
    int opt = 1;
    int fd = 0;
    if(service)
    {
        //service->running_status = 1;	
        /* Setting logger */
        if(service->logfile)
        {
            LOGGER_INIT(service->logger, service->logfile);
            DEBUG_LOGGER(service->logger, "Setting service[%d] log to %s", 
                    service->name, service->logfile);
        }
        if(service->evlogfile)
        {
            LOGGER_INIT(service->evlogger, service->evlogfile);
            DEBUG_LOGGER(service->evlogger, "Setting service[%d] evlog to %s", 
                    service->name, service->evlogfile);
        }
        /* Initialize conns array */
        if(service->max_connections > 0)
            service->connections = (CONN **)calloc(service->max_connections, sizeof(CONN *));
        /* setting as service type */
        if(service->service_type == S_SERVICE)
            goto server_setting;
        else
            goto client_setting;
server_setting:
        DEBUG_LOGGER(service->logger, "Setting service[%s]\n", service->name);
        /* INET setting  */
        if((service->fd = socket(service->family, service->socket_type, 0)) <= 0 )
        {
            FATAL_LOGGER(service->logger, "Initialize socket failed, %s", strerror(errno));
            return -1;
        }
        service->sa.sin_family = service->family;
        service->sa.sin_addr.s_addr = (service->ip)?inet_addr(service->ip):htonl(INADDR_ANY);
        service->sa.sin_port = htons(service->port); 
        //setsockopt 
        setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, (socklen_t)sizeof(opt) );
        //set non-block
        fcntl(service->fd, F_SETFL, O_NONBLOCK);
        //Bind 
        if(bind(service->fd, (struct sockaddr *)&(service->sa), (socklen_t)sizeof(service->sa)) != 0 )
        {
            FATAL_LOGGER(service->logger, "Bind fd[%d] to %s:%d failed, %s",
                    service->fd, service->ip, service->port, strerror(errno));
            return -1;
        }
        //Listen
        if(listen(service->fd, service->backlog) != 0)
        {
            FATAL_LOGGER(service->logger, "Listen fd[%d] On %s:%d failed, %s",
                    service->fd, service->ip, service->port, strerror(errno));	
            return -1;
        }
        /* Event base */
        //service->ev_eventbase = ev_init();
        if(service->evbase == NULL)
        {
            FATAL_LOGGER(service->logger, "Eventbase on fd[%d]  %s:%d is NULL",
                    service->fd, service->ip, service->port);
            return -1;
        }
        if(service->event)
        {
            service->event->set(service->event, service->fd,
                    E_READ | E_PERSIST, (void *)service, service->event_handler);
            service->evbase->add(service->evbase, service->event);
        }
        /*
        if((fd = socket(service->family, service->socket_type, 0)) > 0 
                && connect(fd, (struct sockaddr *)&(service->sa), 
                    (socklen_t)sizeof(struct sockaddr)) == 0)
        {
            DEBUG_LOGGER(service->logger, "Connecting via %d to %s:%d test successed",
                    fd, service->ip, service->port);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            return 0;
        }
        else
        {
            FATAL_LOGGER(service->logger, "Connecting via fd[%d]  to %s:%d is failed, %s",
                    fd, service->ip, service->port, strerror(errno));
            return -1;
        }*/
        return 0;
client_setting:
        DEBUG_LOGGER(service->logger, "Setting client[%s]\n", service->name);
        service->sa.sin_family = service->family;
        service->sa.sin_addr.s_addr = (service->ip)?inet_addr(service->ip):htonl(INADDR_ANY);
        service->sa.sin_port = htons(service->port); 
        return 0;
    }
    return -1;
}
#ifdef HAVE_PTHREAD
#define NEW_PROCTHREAD(id, pthid, pth, logger)                                              \
{                                                                                           \
    if(pthread_create(&pthid, NULL, (void *)(pth->run), (void *)pth) == 0)                  \
    {                                                                                       \
        DEBUG_LOGGER(logger, "Created procthreads[%d] ID[%08x]", id, pthid);                \
    }	                                                                                    \
    else                                                                                    \
    {                                                                                       \
        FATAL_LOGGER(logger, "Create procthreads[%d] failed, %s", id, strerror(errno));     \
        exit(EXIT_FAILURE);				                                                    \
    }                                                                                       \
}
#else
#define NEW_PROCTHREAD(id, pthid, pth, logger)
#endif
/* Run service */
void service_run(SERVICE *service)
{
    int i = 0, fd = 0;
#ifdef HAVE_PTHREAD
    pthread_t procthread_id;
#endif
    /* Running */
    if(service)
    {
        DEBUG_LOGGER(service->logger, "service[%s] using WORKING_MODE[%d]", 
                service->name, service->working_mode);
#ifdef	HAVE_PTHREAD
        if(service->working_mode == WORKING_PROC) 
            goto work_proc_init;
        else 
            goto work_thread_init;
        return ;
#endif
work_proc_init:
        /* Initialize procthread(s) */
        if((service->procthread = procthread_init()))
        {
            service->procthread->logger = service->logger;
            service->procthread->evbase = service->evbase;
            if(service->message_queue && service->procthread->message_queue)
            {
                CLEAN_QUEUE(service->procthread->message_queue);
                service->procthread->message_queue = service->message_queue;
                DEBUG_LOGGER(service->logger, 
                        "Replaced procthread[%08x]->message_queue with service[%08x]->message_queue[%08x]",
                        service->procthread, service, service->message_queue);
            }
            service->procthread->service = service;
            service->running_status = 1;
        }
        else
        {
            FATAL_LOGGER(service->logger, "Initialize procthreads failed, %s",
                    strerror(errno));
            exit(EXIT_FAILURE);
        }	
        goto end;
work_thread_init:
        /* Initialize Threads */
        //threads
        if(service->max_procthreads > 0)
        {
            service->procthreads = (PROCTHREAD **)calloc(
                    service->max_procthreads, sizeof(PROCTHREAD *));
            if(service->procthreads == NULL)
            {
                FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
                        strerror(errno));
                exit(EXIT_FAILURE);
            }
            for(i = 0; i < service->max_procthreads; i++)
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
                NEW_PROCTHREAD(i, procthread_id, service->procthreads[i], service->logger);
            }
        }
        //daemons
        if(service->max_daemons > 0) 
        {
            service->daemons = (PROCTHREAD **)calloc(service->max_daemons,sizeof(PROCTHREAD *));
            if(service->daemons == NULL)
            {
                FATAL_LOGGER(service->logger, "Initialize daemon procthreads pool failed, %s",
                        strerror(errno));
                exit(EXIT_FAILURE);
            }
            for(i = 0; i < service->max_daemons; i++)
            {
                if((service->daemons[i] = procthread_init()))
                {
                    service->daemons[i]->service = service;
                    service->daemons[i]->logger = service->logger;
                    service->daemons[i]->evbase = service->evbase;
                }
                else
                {
                    FATAL_LOGGER(service->logger, "Initialize daemon procthreads pool failed, %s",
                            strerror(errno));
                    exit(EXIT_FAILURE);
                }
                NEW_PROCTHREAD(i, procthread_id, service->daemons[i], service->logger);
            }
        }
        goto end ;
        /* client connection initialize */
end:
        /*
           if(service->service_type == S_SERVICE)
           {
           service->newconn(service, service->ip, service->port); 
           }
           */
        /*
           if(service->service_type == C_SERVICE)
           {
           for(i = 0; i < service->connections_limit; i++)
           {
           if(service->newconn(service, NULL, -1))
           {
           DEBUG_LOGGER(service->logger, "Connected to %s:%d via %d",
           service->ip, service->port, fd);
           }
           else
           {
           ERROR_LOGGER(service->logger, "Connectting to %s:%d via %d failed, %s",
           service->ip, service->port, fd, strerror(errno));
           break;
           }
           }
           }
           */
        return ; 
    }
}

/* Event handler */
void service_event_handler(int event_fd, short event, void *arg)
{
    struct sockaddr_in rsa;
    int fd = 0;
    socklen_t rsa_len = sizeof(rsa);
    SERVICE *service = (SERVICE *)arg;
    if(service)
    {
        if(event_fd != service->fd) 
        {
            FATAL_LOGGER(service->logger, "Invalid event_fd[%d] not match daemon fd[%d]",
                    event_fd, service->fd);
            return ;
        }
        if(event & E_READ)
        {
            if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0 )	
            {
                DEBUG_LOGGER(service->logger, "Accept new connection[%d] from %s:%d",
                        fd, inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port));			
                service->addconn(service, fd, &rsa, NULL);
                return ;
            }
            else
            {
                FATAL_LOGGER(service->logger, "Accept new connection[%d] failed, %s",
                        fd, strerror(errno));
            }
        }
    }
    return ;
}

/* Add new conn */
CONN * service_addconn(SERVICE *service, int fd,  struct sockaddr_in *sa, CALLBACK_OPS *ops)
{
    CONN *conn = NULL;
    char *ip = NULL;
    int port = 0;
    int index = 0;
    if(service)
    {
        /*Check Connections Count */
        if(service->running_connections >= service->max_connections)
        {
            ERROR_LOGGER(service->logger, "Connections count[%d] reach max[%d]",
                    service->running_connections, service->max_connections);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            return conn;
        }
        /* Initialize connection */
        ip   = inet_ntoa(sa->sin_addr);
        port = ntohs(sa->sin_port);
        if((conn = conn_init(ip, port)))
        {        
            conn->fd = fd;
            conn->logger = service->logger;
            conn->packet_type = service->packet_type;
            conn->packet_length = service->packet_length;
            conn->packet_delimiter = service->packet_delimiter;
            conn->packet_delimiter_length = service->packet_delimiter_length;
            conn->buffer_size = service->buffer_size;
            if(ops)
                memcpy(&(conn->ops), ops, sizeof(CALLBACK_OPS));
            else
                memcpy(&(conn->ops), &(service->ops), sizeof(CALLBACK_OPS));
            /* other option */
            conn->timeout = service->conn_timeout;
        }
        else return conn;
        /* Add connection for procthread */
        if(service->working_mode == WORKING_PROC && service->procthread)
        {
            DEBUG_LOGGER(service->logger, "Adding connection[%d] on %s:%d to procthread[%d]",
                    conn->fd, conn->ip, conn->port, getpid());
            service->procthread->addconn(service->procthread, conn);
            return conn;
        }
        //fprintf(stdout, "%dOK:%08x %s:%d\n", __LINE__, conn, ip, port);
        /* Add connection to procthread pool */
        if(service->working_mode == WORKING_THREAD && service->procthreads)
        {
            index = conn->fd % service->max_procthreads;
            DEBUG_LOGGER(service->logger, "Adding connection[%d] on %s:%d to procthreads[%d]",
                    conn->fd, conn->ip, conn->port, index);
            service->procthreads[index]->addconn(service->procthreads[index], conn);
            return conn;
        }
    }
    return conn;
}

/* check service hearbeat handler */
void service_active_heartbeat(SERVICE *service)
{
    if(service)
    {
        if(service->heartbeat_interval > 0
                && TIMER_CHECK(service->timer, service->heartbeat_interval) == 0)
        {
            if(service->cb_heartbeat_handler)
            {
                service->cb_heartbeat_handler(service->cb_heartbeat_arg);
            }
            service->state_conns(service);
            TIMER_SAMPLE(service->timer);
        }
    }
    return ;
}

/* add new task */
void service_newtask(SERVICE *service, FUNCALL taskhandler, void *arg)
{
    PROCTHREAD *pth = NULL;
    int index = 0;
    if(service)
    {
        /* Add task for procthread */
        if(service->working_mode == WORKING_PROC && service->procthread)
            pth = service->procthread;
        if(service->working_mode == WORKING_THREAD && service->daemons)
        {
            index = service->ntask % service->max_daemons;
            pth = service->daemons[index];
        }
        if(pth)
        {
            pth->newtask(pth, taskhandler, arg);
            service->ntask++;
        }
    }
    return ;
}

/* add new transaction */
void service_newtransaction(SERVICE *service, CONN *conn, int tid)
{
    PROCTHREAD *pth = NULL;
    int index = 0;

    if(service && conn && conn->fd > 0)
    {
        /* Add transaction for procthread */
        if(service->working_mode == WORKING_PROC && service->procthread)
        {
            DEBUG_LOGGER(service->logger, "Adding transaction[%d] on %s:%d to procthread[%d]",
                    tid, conn->ip, conn->port, getpid());
            return service->procthread->newtransaction(service->procthread, conn, tid);
        }
        /* Add transaction to procthread pool */
        if(service->working_mode == WORKING_THREAD && service->procthreads)
        {
            index = conn->fd % service->max_procthreads;
            pth = service->procthreads[index];
            DEBUG_LOGGER(service->logger, "Adding transaction[%d] on %s:%d to procthreads[%d]",
                    tid, conn->ip, conn->port, index);
            if(pth && pth->newtransaction)
                return pth->newtransaction(pth, conn, tid);
        }
    }
    return ;
}

/* check conns status */
void service_state_conns(SERVICE *service)
{
    CONN *conn = NULL;
    int i = 0, num = 0;
    if(service)
    {
        if(service->service_type == C_SERVICE
                && service->running_connections < service->connections_limit)
        {
            num = service->connections_limit - service->running_connections;
            DEBUG_LOGGER(service->logger, "limit:%d running:%d", 
                    service->connections_limit, service->running_connections);
            //       num, service->running_connections);
            while(i++ < num)
            {
                if(service->newconn(service, -1, NULL, -1, -1, NULL) == NULL)
                {
                    break;
                }
            }
        }
        i = 0;
        while(i < service->running_connections)
        {
            if((conn = service->connections[i++]) && conn->timeout > 0 
                    && conn->timer && TIMER_CHECK(conn->timer, conn->timeout) == 0)
            {
                conn->push_message(conn, MESSAGE_STATE);
            }
        }
    }
}

/* NEW connection for client mode */
CONN *service_newconn(SERVICE *service, int family, char *ip, 
        int port, int sock_type, CALLBACK_OPS *ops)
{
    CONN *conn = NULL;
    struct sockaddr_in sa, *psa = NULL, lsa;
    socklen_t lsa_len = sizeof(lsa);
    int fd = 0, i = 0;

    if(service)
    {
        if(ip && port > 0)
        {
            if(family == -1) sa.sin_family = service->family;
            else sa.sin_family = family;
            sa.sin_addr.s_addr = inet_addr(ip);
            sa.sin_port = htons(port);
            psa = &sa;
        }
        else
        {
            psa = &(service->sa);
        }
        if(sock_type == -1)
            fd = socket(service->family, service->socket_type, 0);
        else
            fd = socket(service->family, sock_type, 0);
        //fcntl(fd, F_SETFL, O_NONBLOCK);
        if(fd > 0 && connect(fd, (struct sockaddr *)psa, sizeof(sa)) == 0)
        //if(fd > 0 && (connect(fd, (struct sockaddr *)psa,
         //               sizeof(struct sockaddr )) == 0 || errno == EINPROGRESS))
        {
            DEBUG_LOGGER(service->logger, "Ready for connection %s:%d via %d",
                    inet_ntoa(psa->sin_addr), ntohs(psa->sin_port), fd);
            getsockname(fd, (struct sockaddr *)&(lsa), &(lsa_len));
               conn = service->addconn(service, fd, &lsa, ops);
            /*
            if((conn = service->addconn(service, fd, &lsa, ops)))
                conn->status = CONN_STATUS_READY;
               */
        }
        else
        {
            ERROR_LOGGER(service->logger, "Connectting to %s:%d via %d failed, %s",
                    inet_ntoa(psa->sin_addr), ntohs(psa->sin_port), fd, strerror(errno));
            if(fd > 0)close(fd);
        }
    }
    return conn;
}

/* POP connections from connections pool */
void service_popconn(SERVICE *service, int index)
{
    if(service)
    {
        MUTEX_LOCK(service->mutex);
        if(service->connections && index >= 0)
        {
            DEBUG_LOGGER(service->logger, "Pop connection[%d] %s:%d index[%d]", 
                    service->connections[index]->fd, service->connections[index]->ip,
                    service->connections[index]->port, index);
            service->connections[index] = NULL;
            service->running_connections--;
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return ;
}

/* PUSH connections to connections pool */
void service_pushconn(SERVICE *service, CONN *conn)
{
    int i = 0;
    if(service && conn)
    {
        MUTEX_LOCK(service->mutex);
        while(i < service->max_connections && service->connections)
        {
            if(service->connections[i] == NULL)
            {
                service->connections[i] = conn;
                conn->index = i;
                service->running_connections++;
                break;
            }
            i++;
        }
        DEBUG_LOGGER(service->logger, "Pushed connection[%d] %s:%d to index[%d]", 
                conn->fd, conn->ip, conn->port, conn->index);
        MUTEX_UNLOCK(service->mutex);
    }
    return ;
}

/* get free connection */
CONN *service_getconn(SERVICE *service)
{
    int i = 0;
    int fd = 0;
    CONN *conn = NULL;

    if(service && service->connections && service->running_connections > 0)
    {
        MUTEX_LOCK(service->mutex);
        //select free connection 
        for( i = 0; i < service->max_connections; i++)
        {
            conn = service->connections[i];
            if(conn && conn->status != CONN_STATUS_READY && conn->start_cstate(conn) == 0)
            {
                break;
            }
            conn = NULL;
        }
        MUTEX_UNLOCK(service->mutex);
    }
    return conn;
}

/* Terminate service */
void service_terminate(SERVICE *service)
{
    int i = 0;
    PROCTHREAD *pth = NULL;
    if(service)
    {
        service->running_status = 0;	
        if(service->working_mode == WORKING_PROC)
        {
            service->procthread->terminate(service->procthread);
        }
        else
        {
            for(i = 0; i < service->max_procthreads; i++)
            {
                pth = service->procthreads[i];
                pth->terminate(pth);
            }
        }
        shutdown(service->fd, SHUT_RDWR);
        close(service->fd);
        service->fd = -1;
        return;
    }
}

/* Clean service */
void service_clean(SERVICE **service)
{
    int i = 0;
    if((*service))
    {
        if((*service)->working_mode == WORKING_PROC)
        {
            (*service)->procthread->clean(&(*service)->procthread);
        }	
        else
        {
            if((*service)->procthreads)
            {
                for(i = 0; i < (*service)->max_procthreads; i++)
                {
                    (*service)->procthreads[i]->clean(&((*service)->procthreads[i]));
                }
                free((*service)->procthreads);
            }
            if((*service)->daemons)
            {
                for(i = 0; i < (*service)->max_daemons; i++)
                {
                    (*service)->daemons[i]->clean(&((*service)->daemons[i]));
                }
                free((*service)->daemons);
            }
        }
        if((*service)->connections)
        {
            free((*service)->connections);
            (*service)->connections = NULL;
        }
        if((*service)->logger) 
            LOGGER_CLEAN((*service)->logger);
        if((*service)->evlogger) 
            LOGGER_CLEAN((*service)->evlogger);
        if((*service)->event) 
            (*service)->event->clean(&(*service)->event);
        TIMER_CLEAN((*service)->timer);
        if((*service)->mutex)
            MUTEX_DESTROY((*service)->mutex);
        free((*service));
        (*service) = NULL;
    }		
}
