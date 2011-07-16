#include "sbase.h"
#include "service.h"
#include "procthread.h"
#include "xqueue.h"
#include "logger.h"
#include "message.h"
#include "evtimer.h"
#include "chunk.h"
#include "stime.h"
#include "mutex.h"
#include "xmm.h"
#include "mmblock.h"
#define PUSH_TASK_MESSAGE(pth, msgid, index, fd, tid, handler, arg)                         \
do                                                                                          \
{                                                                                           \
    qmessage_push(pth->message_queue, msgid, index, fd, tid, pth, handler, arg);            \
    MUTEX_SIGNAL(pth->mutex);                                                               \
}while(0)

/* event handler */
void procthread_event_handler(int event_fd, int flag, void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg, *parent = NULL;
    char buf[SB_BUF_SIZE], *p = NULL, *ip = NULL;
    socklen_t rsa_len = sizeof(struct sockaddr_in);
    int fd = -1, port = -1, n = 0, opt = 1;
    SERVICE *service = NULL;
    struct sockaddr_in rsa;
    CONN *conn = NULL;
    void *ssl = NULL;

    if(pth && (service = pth->service))
    {
        if(event_fd == service->fd)
        {
            if(E_READ & flag)
            {
                if(service->sock_type == SOCK_STREAM)
                {
                    //WARN_LOGGER(service->logger, "new-connection via %d", service->fd);
                    while((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0)
                    {
                        ip = inet_ntoa(rsa.sin_addr);
                        port = ntohs(rsa.sin_port);
#ifdef HAVE_SSL
                        if(service->is_use_SSL && service->s_ctx)
                        {
                            if((ssl = SSL_new(XSSL_CTX(service->s_ctx))) && SSL_set_fd((SSL *)ssl, fd) > 0 
                                    && SSL_accept((SSL *)ssl) > 0)                                                   
                            {
                                goto new_conn;
                            }
                            else goto err_conn; 
                        }
#endif
new_conn:
                        if((conn = service_addconn(service, service->sock_type, fd, ip, port, 
                                        service->ip, service->port, &(service->session), 
                                        ssl, CONN_STATUS_FREE)))
                        {
                            DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d]  via %d", ip, port, fd);
                            continue;
                        }
                        else
                        {
                            WARN_LOGGER(service->logger, "accept newconnection[%s:%d]  via %d failed, %s", ip, port, fd, strerror(errno));
                        }
err_conn:               
#ifdef HAVE_SSL
                        if(ssl)
                        {
                            SSL_shutdown((SSL *)ssl);
                            SSL_free((SSL *)ssl);
                            ssl = NULL;
                        }
#endif
                        if(fd > 0)
                        {
                            shutdown(fd, SHUT_RDWR);
                            close(fd);
                        }
                    }
                }
                else if(service->sock_type == SOCK_DGRAM)
                {
                    while((n = recvfrom(event_fd, buf, SB_BUF_SIZE, 
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
                                        ip, port, service->ip, service->port, 
                                        &(service->session), NULL, CONN_STATUS_FREE)))
                        {
                            p = buf;
                            MMB_PUSH(conn->buffer, p, n);
                            parent = (PROCTHREAD *)(conn->parent);
                            if(parent)
                            {
                                qmessage_push(parent->message_queue, 
                                        MESSAGE_INPUT, -1, conn->fd, -1, conn, parent, NULL);
                                MUTEX_SIGNAL(parent->mutex);
                            }
                            DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d] via %d"
                                    " buffer:%d", ip, port, fd, MMB_NDATA(conn->buffer));
                        }
                        else
                        {
                            shutdown(fd, SHUT_RDWR);
                            close(fd);
                            FATAL_LOGGER(service->logger, "Accept new connection failed, %s", 
                                    strerror(errno));
                        }
                    }
                }
            }
        }
        if(flag & E_WRITE)
        {
            event_del(&(pth->event), E_WRITE);
        }
    }
    return ;
}

/* wakeup */
void procthread_wakeup(PROCTHREAD *pth)
{
    if(pth)
    {
        if(pth->have_evbase && pth->evbase) 
        {
            if(pth->cond == pth->service->fd)
            {
                event_set(&(pth->evcond), pth->service->cond, E_PERSIST|E_READ|E_WRITE, (void *)pth, 
                        &procthread_event_handler);
                pth->evbase->add(pth->evbase, &(pth->evcond));
            }
            else
            {
                event_add(&(pth->event), E_WRITE);
            }
        }
        MUTEX_SIGNAL(pth->mutex);
    }
    return ;
}

/* active */
void procthread_active(PROCTHREAD *pth)
{
    if(pth)
    {
        if(pth->have_evbase && pth->evbase) 
        {
            event_del(&(pth->event), E_WRITE);
        }
    }
    return ;
}
 
/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg;
    struct timeval tv = {0,0};
    int i = 0;

    if(pth)
    {
        DEBUG_LOGGER(pth->logger, "Ready for running thread[%p]", (void*)((long)(pth->threadid)));
        pth->running_status = 1;
        if(pth->usec_sleep > 1000000) tv.tv_sec = pth->usec_sleep/1000000;
        tv.tv_usec = pth->usec_sleep % 1000000;
        if(pth->have_evbase)
        {
            do
            {
                //if(pth->evtimer){EVTIMER_CHECK(pth->evtimer);}
                //DEBUG_LOGGER(pth->logger, "starting evbase->loop(%d)", pth->evbase->efd);
                i = pth->evbase->loop(pth->evbase, 0, NULL);
                if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                {
                    //DEBUG_LOGGER(pth->logger, "starting qmessage_handler()");
                    qmessage_handler(pth->message_queue, pth->logger);
                    //DEBUG_LOGGER(pth->logger, "over qmessage_handler()");
                    i = 1;
                }
                //if(i < 1) usleep(pth->usec_sleep);
                //if(i < 1)++k;else k = 0;
                //fprintf(stdout, "%s::%d i:%d\n", __FILE__, __LINE__, i);
                //if(k  > 10000){usleep(5000); k = 0;}
            }while(pth->running_status);
            DEBUG_LOGGER(pth->logger, "ready to exit iodaemon");
        }
        else
        {
            if(pth->use_cond_wait)
            {
                do
                {
                    //DEBUG_LOGGER(pth->logger, "starting cond-wait() threads[%p]->qmessage[%p]_handler(%d)", (void *)pth->threadid,pth->message_queue, QMTOTAL(pth->message_queue));
                    //if(pth->evtimer){EVTIMER_CHECK(pth->evtimer);}
                    if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                    {
                        //DEBUG_LOGGER(pth->logger, "starting qmessage_handler()");
                        qmessage_handler(pth->message_queue, pth->logger);
                        //DEBUG_LOGGER(pth->logger, "over qmessage_handler()");
                    }
                    else
                    {

                        if(QMTOTAL(pth->message_queue) <= 0){MUTEX_WAIT(pth->mutex);}
                    }
                    //DEBUG_LOGGER(pth->logger, "over cond-wait() threads[%p]->qmessage[%p]_handler(%d)", (void *)pth->threadid,pth->message_queue, QMTOTAL(pth->message_queue));
                }while(pth->running_status);
                DEBUG_LOGGER(pth->logger, "ready to exit threads/daemons[%d]", pth->index);
            }
            else
            {
                do
                {
                    //DEBUG_LOGGER(pth->logger, "starting threads[%p]->qmessage[%p]_handler(%d)", (void *)(pth->threadid),pth->message_queue, QMTOTAL(pth->message_queue));
                    if(pth->evtimer){EVTIMER_CHECK(pth->evtimer);}
                    if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                    {
                        qmessage_handler(pth->message_queue, pth->logger);
                    }
                    else
                    {
                        ++i;
                    }
                    if(i > 1000){usleep(pth->usec_sleep); i = 0;}
                    //if(i > 1000) select(0, NULL, NULL, NULL, &tv);
                    //DEBUG_LOGGER(pth->logger, "over threads[%p]->qmessage[%p]_handler(%d)", (void *)(pth->threadid),pth->message_queue, QMTOTAL(pth->message_queue));
                }while(pth->running_status);
                DEBUG_LOGGER(pth->logger, "ready to exit daemon");
            }
        }
        if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                qmessage_handler(pth->message_queue, pth->logger);
        //WARN_LOGGER(pth->logger, "terminate threads[%d][%p] evbase[%p] qmessage[%p] ioqmessage[%p] qtotal:%d", pth->index, (void *)(pth->threadid), pth->evbase, pth->message_queue, pth->ioqmessage, QMTOTAL(pth->message_queue));
    }
#ifdef HAVE_PTHREAD
    pthread_exit(NULL);
#endif
    return ;
}

/* push new connection  */
int procthread_pushconn(PROCTHREAD *pth, int fd, void *ssl)
{
    int ret = -1;

    if(pth && pth->message_queue && fd > 0)
    {
        PUSH_TASK_MESSAGE(pth,MESSAGE_NEW_CONN, -1, fd, -1, ssl, NULL);
        DEBUG_LOGGER(pth->logger, "Added message[NEW_CONN][%d] to procthreads[%d] qtotal:%d", fd, pth->index, QMTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* add new task */
int procthread_newtask(PROCTHREAD *pth, CALLBACK *task_handler, void *arg)
{
    int ret = -1;

    if(pth && pth->message_queue && task_handler && pth->lock == 0)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_TASK, -1, -1, -1, task_handler, arg);
        DEBUG_LOGGER(pth->logger, "Added message task to procthreads[%d]", pth->index);
        ret = 0;
    }
    return ret;
}

/* add new transaction */
int procthread_newtransaction(PROCTHREAD *pth, CONN *conn, int tid)
{
    int ret = -1;

    if(pth && pth->message_queue && conn && pth->lock == 0)
    {
        PUSH_TASK_MESSAGE(pth,MESSAGE_TRANSACTION, -1, conn->fd, tid, conn, NULL);
        DEBUG_LOGGER(pth->logger, "Added thread[%p]->qmessage[%p] transaction[%d] to conn[%p][%s:%d] on %s:%d via %d total %d", (void *)(pth->threadid),  pth->message_queue, tid, conn, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd, QMTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* new connection */
int procthread_newconn(PROCTHREAD *pth, int fd, void *ssl)
{
    socklen_t rsa_len = sizeof(struct sockaddr_in);
    struct sockaddr_in rsa;
    SERVICE *service = NULL;
    int port = 0, ret = -1;
    CONN *conn = NULL;
    char *ip = NULL;

    if(pth && fd > 0 && (service = pth->service))
    {
        if(getpeername(fd, (struct sockaddr *)&rsa, &rsa_len) == 0
                && (ip = inet_ntoa(rsa.sin_addr)) && (port = ntohs(rsa.sin_port)) > 0 
                && (conn = service_addconn(service, service->sock_type, fd, ip, port, 
                    service->ip, service->port, &(service->session), ssl, CONN_STATUS_FREE)))
        {
            DEBUG_LOGGER(pth->logger, "adding new-connection[%p][%s:%d] via %d", conn, ip, port, fd);
            ret = 0;
        }
        else
        {
#ifdef HAVE_SSL
            if(ssl)
            {
                SSL_shutdown((SSL *)ssl);
                SSL_free((SSL *)ssl);
                ssl = NULL;
            }
#endif
            DEBUG_LOGGER(pth->logger, "adding new-connection[%d] failed,%s", fd, strerror(errno));
            shutdown(fd, SHUT_RDWR);
            close(fd);
        }
        return ret;
    }
    return -1;
}

/* Add connection message */
int procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && pth->message_queue && conn  && pth->lock == 0)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_NEW_SESSION, -1, conn->fd, -1, conn, NULL);
        DEBUG_LOGGER(pth->logger, "Ready for adding msg[%s] connection[%p][%s:%d] d_state:%d "
                "on %s:%d via %d total %d", MESSAGE_DESC(MESSAGE_NEW_SESSION), conn,
                conn->remote_ip, conn->remote_port, conn->d_state, conn->local_ip, conn->local_port,
                conn->fd, QMTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* Add new connection */
int procthread_add_connection(PROCTHREAD *pth, CONN *conn)
{
    PROCTHREAD *iodaemon = NULL;
    int ret = -1;

    if(pth && conn)
    {
        DEBUG_LOGGER(pth->logger, "Ready for add connection[%p][%s:%d] d_state:%d "
                " on %s:%d via %d to pool", conn, conn->remote_ip, conn->remote_port,
                conn->d_state, conn->local_ip, conn->local_port, conn->fd);
        conn->message_queue = pth->message_queue;
        conn->ioqmessage    = pth->ioqmessage;
        conn->iodaemon      = pth->iodaemon;
        conn->evbase        = pth->evbase;
        conn->parent        = pth;
        conn->queue         = pth->queue;
        //conn->reset_state(conn);
        if(pth->service->pushconn(pth->service, conn) == 0 && conn->set(conn) == 0)
        {
            DEBUG_LOGGER(pth->logger, "Ready for add conn[%p][%s:%d] d_state:%d "
                " on %s:%d via %d to pool", conn, conn->remote_ip, conn->remote_port, 
                conn->d_state, conn->local_ip, conn->local_port, conn->fd);
        }
        if((iodaemon = (PROCTHREAD *)pth->iodaemon)) iodaemon->wakeup(iodaemon);
    }
    return ret;
}

/* procthread over connection */
int procthread_over_connection(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && pth->service)
    {
        event_destroy(&(conn->event));
        pth->service->overconn(pth->service, conn);
        ret = 0;
    }
    return ret;
}

/* procthread shut connection */
int procthread_shut_connection(PROCTHREAD *pth, CONN *conn)
{
    PROCTHREAD *iodaemon = NULL;
    int ret = -1;

    if(pth && pth->service)
    {
        if((iodaemon = pth->iodaemon) && iodaemon->message_queue)
        {
            qmessage_push(iodaemon->message_queue, MESSAGE_OVER, conn->index, conn->fd, 
                -1, iodaemon, conn, NULL);
            iodaemon->wakeup(iodaemon);
        }
        else
        {
            qmessage_push(pth->message_queue, MESSAGE_QUIT, conn->index, conn->fd,
                    -1, pth, conn, NULL);
        }
        ret = 0;
    }
    return ret;
}

/* Terminate connection */
int procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && conn)
    {
        ret = pth->service->popconn(pth->service, conn);
        ret = conn->terminate(conn);
        if(pth->lock)
        {
            DEBUG_LOGGER(pth->logger, "Ready for clean conn[%p]", conn);
            conn->clean(conn);
        }
        else
        {
            conn->reset(conn);
            service_pushtoq(pth->service, conn);
        }
    }
    return ret;
}

/* stop procthread */
void procthread_stop(PROCTHREAD *pth)
{
    if(pth)
    {
        if(pth->message_queue)
        {
            pth->lock       = 1;
            PUSH_TASK_MESSAGE(pth, MESSAGE_STOP, -1, -1, -1, NULL, NULL);
            pth->wakeup(pth);
            WARN_LOGGER(pth->logger, "Pushed MESSAGE_QUIT to procthreads[%d]", pth->index);
        }
        else
        {
            pth->lock       = 1;
            pth->running_status = 0;
            pth->wakeup(pth);
        }
        //WARN_LOGGER(pth->logger, "Ready for stopping procthreads[%d] evbase[%p] iodaemon[%p] ioqmessage[%p] qmessage[%p]", pth->index, pth->evbase, pth->iodaemon, pth->ioqmessage, pth->message_queue);
    }
    return ;
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
    if(pth)
    {
        WARN_LOGGER(pth->logger, "Ready for closing procthread[%d]", pth->index);
        pth->lock       = 1;
        pth->running_status = 0;
        pth->wakeup(pth);
        //WARN_LOGGER(pth->logger, "Ready for closing procthreads[%d] evbase[%p] iodaemon[%p] ioqmessage[%p] qmessage[%p]", pth->index, pth->evbase, pth->iodaemon, pth->ioqmessage, pth->message_queue);
    }
    return ;
}

/* state */
void procthread_state(PROCTHREAD *pth,  CALLBACK *handler, void *arg)
{
    if(pth && pth->message_queue)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_STATE, -1, -1, -1, handler, arg);
        //DEBUG_LOGGER(pth->logger, "Ready for state connections on daemon procthread");
    }
    return ;
}

/* active heartbeat */
void procthread_active_heartbeat(PROCTHREAD *pth,  CALLBACK *handler, void *arg)
{
    if(pth && pth->message_queue)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_HEARTBEAT, -1, -1, -1, handler, arg);
        //DEBUG_LOGGER(pth->logger, "Ready for activing heartbeat on daemon procthread");
    }
    return ;

}

/* clean procthread */
void procthread_clean(PROCTHREAD *pth)
{
    if(pth)
    {
        if(pth->service->working_mode != WORKING_PROC)
        {
            if(pth->have_evbase)
            {
                event_clean(&(pth->event));
                event_clean(&(pth->evcond));
                if(pth->evbase) pth->evbase->clean(pth->evbase);
            }
            qmessage_clean(pth->message_queue);
        }
        MUTEX_DESTROY(pth->mutex);
        xmm_free(pth, sizeof(PROCTHREAD));
    }
    return ;
}

/* Initialize procthread */
PROCTHREAD *procthread_init(int cond)
{
    PROCTHREAD *pth = NULL;

    if((pth = (PROCTHREAD *)xmm_mnew(sizeof(PROCTHREAD))))
    {
        if(cond  > 0)
        {
            pth->have_evbase = 1;
            /*
            struct ip_mreq mreq;
            memset(&mreq, 0, sizeof(struct ip_mreq));
            mreq.imr_multiaddr.s_addr = inet_addr("239.239.239.239");
            mreq.imr_interface.s_addr = inet_addr("127.0.0.1");
            if((pth->evbase = evbase_init()) 
                && (pth->cond = socket(AF_INET, SOCK_DGRAM, 0)) > 0 
                && setsockopt(pth->cond, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, 
                    sizeof(struct ip_mreq)) == 0)
            */
            if((pth->evbase = evbase_init())) 
            {
                //pth->evbase->set_evops(pth->evbase, EOP_POLL);
                pth->cond = cond;
                event_set(&(pth->event), pth->cond, E_PERSIST|E_READ|E_WRITE, (void *)pth, 
                        &procthread_event_handler);
                pth->evbase->add(pth->evbase, &(pth->event));
            }
            else 
            {
                fprintf(stderr, "evbase_init failed, %s\n", strerror(errno));
                _exit(-1);
            }
        }
        MUTEX_RESET(pth->mutex);
        pth->message_queue          = qmessage_init();
        pth->run                    = procthread_run;
        pth->pushconn               = procthread_pushconn;
        pth->newconn                = procthread_newconn;
        pth->addconn                = procthread_addconn;
        pth->add_connection         = procthread_add_connection;
        pth->newtask                = procthread_newtask;
        pth->newtransaction         = procthread_newtransaction;
        pth->shut_connection        = procthread_shut_connection;
        pth->over_connection        = procthread_over_connection;
        pth->terminate_connection   = procthread_terminate_connection;
        pth->stop                   = procthread_stop;
        pth->wakeup                 = procthread_wakeup;
        pth->active                 = procthread_active;
        pth->terminate              = procthread_terminate;
        pth->state                  = procthread_state;
        pth->active_heartbeat       = procthread_active_heartbeat;
        pth->clean                  = procthread_clean;
    }
    return pth;
}
