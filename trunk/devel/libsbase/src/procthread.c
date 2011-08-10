#include "sbase.h"
#include "service.h"
#include "procthread.h"
#include "logger.h"
#include "message.h"
#include "evtimer.h"
#include "chunk.h"
#include "stime.h"
#include "mutex.h"
//#include "xqueue.h"
#include "xmm.h"
#include "mmblock.h"
#define PUSH_TASK_MESSAGE(pth, msgid, index, fd, tid, handler, arg)                         \
do                                                                                          \
{                                                                                           \
    qmessage_push(pth->message_queue, msgid, index, fd, tid, pth, handler, arg);            \
    pth->wakeup(pth);                                                                       \
}while(0)
/* event handler */
void procthread_event_handler(int event_fd, int flags, void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg;
    SERVICE *service = NULL;

    if(pth && (service = pth->service))
    {
        if(event_fd == service->fd)
        {
            service_accept_handler(service);
        }
        else
        {
            if(QMTOTAL(pth->message_queue) < 1) 
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
            event_add(&(pth->event), E_WRITE);
        }
        else
        {
            if(pth->service->flag & SB_USE_EVSIG)
                evsig_wakeup(&(pth->evsig));
        }
        //if(pth->use_cond_wait){MUTEX_SIGNAL(pth->mutex);}
    }
    return ;
}

/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg;
    int i = 0, usec = 0, sec = 0;
    struct timeval tv = {0,0};
    struct timespec ts = {0, 0};
    int k = 0, n = 0;
    char line[1024];

    if(pth)
    {
        DEBUG_LOGGER(pth->logger, "Ready for running thread[%p]", (void*)((long)(pth->threadid)));
        pth->running_status = 1;
        if(pth->usec_sleep > 1000000) sec = pth->usec_sleep/1000000;
        usec = pth->usec_sleep % 1000000;

        tv.tv_sec = sec;
        tv.tv_usec = usec;
        ts.tv_sec = sec;
        ts.tv_nsec = (long)usec * 1000l;
        if(pth->have_evbase)
        {
            if(pth->cond > 0)
            {
                event_set(&(pth->event), pth->cond, E_READ|E_PERSIST,
                        (void *)pth, (void *)&procthread_event_handler);
                pth->evbase->add(pth->evbase, &(pth->event));
                /*
                if(pth->service->logger && PLOG(pth->service->logger)->level > 0)
                {
                    if(pth == pth->service->outdaemon)
                    {
                        sprintf(line, "/tmp/%s_outdaemon.log", pth->service->service_name);
                        evbase_set_logfile(pth->evbase, line);
                    }
                    else
                    {
                        sprintf(line, "/tmp/%s_indaemon.log", pth->service->service_name);
                        evbase_set_logfile(pth->evbase, line);
                    }
                }
                */
            }
            do
            {
                i = pth->evbase->loop(pth->evbase, 0, NULL);
                if(pth->message_queue && (k = QMTOTAL(pth->message_queue)) > 0)
                {
                    qmessage_handler(pth->message_queue, pth->logger);
                }
                if(pth->service->flag & SB_LOG_THREAD)
                {
                    if(pth == pth->service->outdaemon)
                    {
                        WARN_LOGGER(pth->logger, "outdaemon_loop(%d/%d) q[%p]{total:%d left:%d}", i, k, pth->message_queue, QMTOTAL(pth->message_queue), QNLEFT(pth->message_queue));
                    }
                    else
                    {
                        WARN_LOGGER(pth->logger, "iodaemon_loop(%d/%d) q[%p]{total:%d left:%d}", i, k, pth->message_queue, QMTOTAL(pth->message_queue), QNLEFT(pth->message_queue));
                    }
                }
                if((pth->service->flag & (SB_IO_NANOSLEEP|SB_IO_USLEEP|SB_IO_SELECT)) 
                        && n++ > pth->service->nworking_tosleep)
                {
                    if(pth->service->flag & SB_IO_NANOSLEEP) nanosleep(&ts, NULL); 
                    else if(pth->service->flag & SB_IO_USLEEP) usleep(pth->usec_sleep);
                    else if(pth->service->flag & SB_IO_SELECT) select(0, NULL, NULL, NULL, &tv);
                    n = 0;
                }
            }while(pth->running_status);

        }
        else if(pth->listenfd > 0)
        {
            /*
               event_set(&(pth->acceptor), pth->listenfd, E_READ|E_PERSIST,
               (void *)pth, (void *)&procthread_event_handler);
               pth->evbase->add(pth->evbase, &(pth->acceptor));
               */
            do
            {
                service_accept_handler(pth->service);
                //ACCESS_LOGGER(pth->logger, "acceptor_loop(%d)", i);
                /*
                i = pth->evbase->loop(pth->evbase, 0, NULL);
                if(pth->service->flag & SB_LOG_THREAD)
                {
                    WARN_LOGGER(pth->logger, "iodaemon_loop(%d/%d) q[%p]{total:%d left:%d}", i, k, pth->message_queue, QMTOTAL(pth->message_queue), QNLEFT(pth->message_queue));
                }
                */
            }while(pth->running_status);
            WARN_LOGGER(pth->logger, "Ready for stop threads[acceptor]");
        }
        else
        {
            if(pth->use_cond_wait)
            {
                do
                {
                    //DEBUG_LOGGER(pth->logger, "starting cond-wait() threads[%p]->qmessage[%p]_handler(%d)", (void *)pth->threadid,pth->message_queue, QMTOTAL(pth->message_queue));
                    if(pth->message_queue && (k = QMTOTAL(pth->message_queue)) > 0)
                    {
                        qmessage_handler(pth->message_queue, pth->logger);
                        i = 1;
                    }
                    //if(QMTOTAL(pth->message_queue) < 1){MUTEX_WAIT(pth->mutex);i = 0;}
                    //if(QMTOTAL(pth->message_queue) < 1){evsig_wait(&(pth->evsig));i = 0;}
                    if(QMTOTAL(pth->message_queue) < 1) 
                    {
                        if(pth->service->flag & SB_USE_EVSIG)
                            evsig_wait(&(pth->evsig));
                        else
                            nanosleep(&ts, NULL);
                    }
                    /*
                    //if(i > pth->service->nworking_tosleep)
                    //{usleep(pth->usec_sleep);i = 0;}
                    //{select(0, NULL, NULL, NULL, &tv); i = 0;}
                    //if(k > pth->service->nworking_tosleep){usleep(pth->usec_sleep);i = 0;}
                    if(pth->service->flag & SB_LOG_THREAD)
                    {
                        WARN_LOGGER(pth->logger, "conn_worker[%p]->loop(%d) q[%p]->total:%d/%d nleft:%d daemon:%p", pth, pth->index, pth->message_queue, QMTOTAL(pth->message_queue), k, QNLEFT(pth->message_queue), pth->service->daemon);
                    }
                    */
                }while(pth->running_status);
                //WARN_LOGGER(pth->logger, "ready to exit threads/daemons[%d]", pth->index);
            }
            else
            {
                do
                {
                    //DEBUG_LOGGER(pth->logger, "starting threads[%p]->qmessage[%p]_handler(%d)", (void *)(pth->threadid),pth->message_queue, QMTOTAL(pth->message_queue));
                    //if(pth->evtimer){EVTIMER_CHECK(pth->evtimer);}
                    if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                    {
                        qmessage_handler(pth->message_queue, pth->logger);
                    }
                    //select(0, NULL, NULL, NULL, &tv);
                    usleep(pth->usec_sleep);
                    //ACCESS_LOGGER(pth->logger, "conn_daemon_loop(%d)", pth->index);
                    //if(i > 1000) select(0, NULL, NULL, NULL, &tv);
                    //DEBUG_LOGGER(pth->logger, "over threads[%p]->qmessage[%p]_handler(%d)", (void *)(pth->threadid),pth->message_queue, QMTOTAL(pth->message_queue));
                }while(pth->running_status);
                //WARN_LOGGER(pth->logger, "ready to exit threads/daemons[%d]", pth->index);
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
                "on %s:%d via %d total %d", messagelist[MESSAGE_NEW_SESSION], conn,
                conn->remote_ip, conn->remote_port, conn->d_state, conn->local_ip, conn->local_port,
                conn->fd, QMTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* Add new connection */
int procthread_add_connection(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && conn)
    {
        DEBUG_LOGGER(pth->logger, "Ready for add connection[%p][%s:%d] d_state:%d "
                " on %s:%d via %d to pool", conn, conn->remote_ip, conn->remote_port,
                conn->d_state, conn->local_ip, conn->local_port, conn->fd);
        conn->message_queue = pth->message_queue;
        conn->indaemon      = pth->indaemon;
        conn->inqmessage    = pth->inqmessage;
        conn->evbase        = pth->evbase;
        conn->outdaemon     = pth->outdaemon;
        conn->outqmessage   = pth->outqmessage;
        conn->outevbase     = pth->outevbase;
        conn->parent        = pth;
        //conn->xqueue        = pth->xqueue;
        if(pth->service->pushconn(pth->service, conn) == 0 && conn->set(conn) == 0)
        {
            DEBUG_LOGGER(pth->logger, "Ready for add conn[%p][%s:%d] d_state:%d "
                " on %s:%d via %d to pool", conn, conn->remote_ip, conn->remote_port, 
                conn->d_state, conn->local_ip, conn->local_port, conn->fd);
        }
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
    PROCTHREAD *indaemon = NULL;
    int ret = -1;

    if(pth && pth->service)
    {
        if((indaemon = pth->indaemon) && indaemon->message_queue)
        {
            qmessage_push(indaemon->message_queue, MESSAGE_OVER, conn->index, conn->fd, 
                -1, indaemon, conn, NULL);
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
            pth->running_status = 0;
            PUSH_TASK_MESSAGE(pth, MESSAGE_STOP, -1, -1, -1, NULL, NULL);
        }
        else
        {
            pth->lock       = 1;
            pth->running_status = 0;
            pth->wakeup(pth);
        }
    }
    return ;
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
    if(pth)
    {
        pth->lock       = 1;
        pth->running_status = 0;
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
        //WARN_LOGGER(pth->logger, "Ready for activing heartbeat on daemon procthread");
    }
    return ;

}

/* set acceptor */
void procthread_set_acceptor(PROCTHREAD *pth, int listenfd)
{
    if(pth && listenfd > 0)
    {
        pth->listenfd = listenfd;
    }
    return ;
}

/* set evsig */
void procthread_set_evsig_fd(PROCTHREAD *pth, int fd)
{
    if(pth && fd > 0)
    {
        if(pth->service->flag & SB_USE_EVSIG)
            evsig_set(&(pth->evsig), fd);
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
                event_destroy(&(pth->event));
                //event_destroy(&(pth->acceptor));
                if(pth->evbase) pth->evbase->clean(pth->evbase);
            }
            qmessage_clean(pth->message_queue);
        }
        evsig_close(&(pth->evsig));
        //xqueue_clean(pth->xqueue);
        //MUTEX_DESTROY(pth->mutex);
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
        if((pth->cond = pth->have_evbase = cond) > 0)
        {
            if((pth->evbase   = evbase_init(0)) == NULL)
            {
                fprintf(stderr, "Initialize evbase failed, %s\n", strerror(errno));
                _exit(-1);
            }
        }
        //MUTEX_INIT(pth->mutex);
        //pth->xqueue                 = xqueue_init();
        pth->message_queue          = qmessage_init();
        pth->run                    = procthread_run;
        pth->set_acceptor           = procthread_set_acceptor;
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
        pth->terminate              = procthread_terminate;
        pth->state                  = procthread_state;
        pth->active_heartbeat       = procthread_active_heartbeat;
        pth->clean                  = procthread_clean;
    }
    return pth;
}
