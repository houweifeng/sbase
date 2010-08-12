#include "service.h"
#include "procthread.h"
#include "queue.h"
#include "logger.h"
#include "message.h"
#include "evtimer.h"
#include "chunk.h"
#define PUSH_TASK_MESSAGE(pth, msgid, index, fd, tid, handler, arg)                         \
do                                                                                          \
{                                                                                           \
    qmessage_push(pth->message_queue, msgid, index, fd, tid, pth, handler, arg);            \
    if(pth->use_cond_wait){MUTEX_SIGNAL(pth->mutex);}                                       \
}while(0)
/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg;
    struct timeval tv = {0};
    int i = 0, ret = 0;

    if(pth)
    {
        //fprintf(stdout, "%s::%d OK\n", __FILE__, __LINE__);
//#ifdef HAVE_PTHREAD
//        pthread_detach((pthread_t)(pth->threadid));
//#endif
        //fprintf(stdout, "%s::%d OK\n", __FILE__, __LINE__);
        DEBUG_LOGGER(pth->logger, "Ready for running thread[%p]", (void*)((long)(pth->threadid)));
        pth->running_status = 1;
        tv.tv_usec = SB_USEC_SLEEP;
        if(pth->usec_sleep > 0) tv.tv_usec = pth->usec_sleep;
        //fprintf(stdout, "%s::%d OK\n", __FILE__, __LINE__);
        if(pth->have_evbase)
        {
            do
            {
                i = 0;
                if(pth->evbase->loop(pth->evbase, 0, &tv) > 0) ++i;
                //pth->evbase->loop(pth->evbase, 0, &tv);
                if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                {
                    qmessage_handler(pth->message_queue, pth->logger);
                    ++i;
                }
                if(i == 0){usleep(pth->usec_sleep);}
            }while(pth->running_status);
        }
        else
        {
            if(pth->use_cond_wait)
            {
                do
                {
                    if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                    {
                        qmessage_handler(pth->message_queue, pth->logger);
                    }
                    else
                    {
                        ret = MUTEX_WAIT(pth->mutex);
                    }
                }while(pth->running_status);
            }
            else
            {
                do
                {
                    if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
                    {
                        qmessage_handler(pth->message_queue, pth->logger);
                    }
                    else
                    {
                        usleep(100);
                    }
                }while(pth->running_status);
            }
        }
        if(pth->message_queue && QMTOTAL(pth->message_queue) > 0)
            qmessage_handler(pth->message_queue, pth->logger);
    }
#ifdef HAVE_PTHREAD
    pthread_exit(NULL);
#endif
    return ;
}

/* add new task */
int procthread_newtask(PROCTHREAD *pth, CALLBACK *task_handler, void *arg)
{
    int ret = -1;

    if(pth && pth->message_queue && task_handler)
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

    if(pth && pth->message_queue && conn)
    {
        PUSH_TASK_MESSAGE(pth,MESSAGE_TRANSACTION, -1, conn->fd, tid, conn, NULL);
        DEBUG_LOGGER(pth->logger, "Added message transaction[%d] to %s:%d on %s:%d via %d total %d",
                tid, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                conn->fd, QMTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* Add connection message */
int procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && pth->message_queue && conn)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_NEW_SESSION, -1, conn->fd, -1, conn, NULL);
        DEBUG_LOGGER(pth->logger, "Ready for adding msg[%s] connection[%s:%d] on "
                "%s:%d via %d total %d", MESSAGE_DESC(MESSAGE_NEW_SESSION),
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port,
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
        DEBUG_LOGGER(pth->logger, "Ready for add connection[%s:%d] on %s:%d via %d to pool",
        conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        conn->message_queue = pth->message_queue;
        conn->ioqmessage    = pth->ioqmessage;
        conn->evbase        = pth->evbase;
        conn->parent        = pth;
        if(conn->set(conn) == 0)
        {
            DEBUG_LOGGER(pth->logger, "Ready for add connection[%s:%d] on %s:%d via %d to pool",
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
            ret = pth->service->pushconn(pth->service, conn);
        }
    }
    return ret;
}

/* Terminate connection */
int procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
    int ret = -1;

    if(pth && conn)
    {
        ret = conn->terminate(conn);
        ret = pth->service->popconn(pth->service, conn);
        if(pth->lock)
        {
            conn->clean(&conn);
        }
        else
        {
            service_pushtoq(pth->service, conn);
        }
        //conn->reset(conn);
    }
    return ret;
}

/* stop procthread */
void procthread_stop(PROCTHREAD *pth)
{
    if(pth && pth->message_queue)
    {
        PUSH_TASK_MESSAGE(pth, MESSAGE_STOP, -1, -1, -1, NULL, NULL);
        DEBUG_LOGGER(pth->logger, "Ready for stopping procthread[%d]", pth->index);
        pth->lock       = 1;
    }
    return ;
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
    if(pth)
    {
        DEBUG_LOGGER(pth->logger, "Ready for closing procthread[%d]", pth->index);
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
        //DEBUG_LOGGER(pth->logger, "Ready for activing heartbeat on daemon procthread");
    }
    return ;

}

/* clean procthread */
void procthread_clean(PROCTHREAD **ppth)
{
    if(ppth && *ppth)
    {
        if((*ppth)->service->working_mode != WORKING_PROC)
        {
            if((*ppth)->have_evbase)
            {
                (*ppth)->evbase->clean(&((*ppth)->evbase));
            }
            qmessage_clean((*ppth)->message_queue);
        }
        MUTEX_DESTROY((*ppth)->mutex);
        free((*ppth));
        (*ppth) = NULL;
    }
}

/* Initialize procthread */
PROCTHREAD *procthread_init(int have_evbase)
{
    PROCTHREAD *pth = NULL;

    if((pth = (PROCTHREAD *)calloc(1, sizeof(PROCTHREAD))))
    {
        if(have_evbase)
        {
            pth->have_evbase        = have_evbase;
            pth->evbase             = evbase_init(0);
        }
        MUTEX_INIT(pth->mutex);
        pth->message_queue          = qmessage_init();
        pth->run                    = procthread_run;
        pth->addconn                = procthread_addconn;
        pth->add_connection         = procthread_add_connection;
        pth->newtask                = procthread_newtask;
        pth->newtransaction         = procthread_newtransaction;
        pth->terminate_connection   = procthread_terminate_connection;
        pth->stop                   = procthread_stop;
        pth->terminate              = procthread_terminate;
        pth->state                  = procthread_state;
        pth->active_heartbeat       = procthread_active_heartbeat;
        pth->clean                  = procthread_clean;
    }
    return pth;
}
