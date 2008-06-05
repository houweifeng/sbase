#include "procthread.h"
#include "queue.h"
#include "logger.h"
#include "message.h"

/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *pth = (PROCTHREAD *)arg;

    if(pth)
    {
        pth->running_status = 1;
        while(pth->running_status)
        {
            if(QTOTAL(pth->message_queue) > 0)
                message_handler(pth->message_queue, pth->logger);
            usleep(pth->usec_sleep);
        }
    }
#ifdef HAVE_PTHREAD
    pthread_exit();
#endif
}

/* add new task */
int procthread_newtask(PROCTHREAD *pth, CALLBACK *task_handler, void *arg)
{
    MESSAGE msg = {0};
    int ret = -1;

    if(pth && pth->message_queue && task_handler)
    {
        msg.msg_id      = MESSAGE_TASK;
        msg.parent      = pth;
        msg.handler     = task_handler;
        msg.arg         = arg;
        QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(pth->logger, "Added message task to procthreads[%d]", pth->index);
        ret = 0;
    }
    return ret;
}

/* add new transaction */
int procthread_newtransaction(PROCTHREAD *pth, CONN *conn, int tid)
{
    MESSAGE msg = {0};
    int ret = -1;

    if(pth && pth->message_queue && conn)
    {
        msg.msg_id = MESSAGE_TRANSACTION;
        msg.fd = conn->fd;
        msg.tid = tid;
        msg.handler = conn;
        msg.parent  = (void *)pth;
        QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(pth->logger, "Added message transaction[%d] to %s:%d via %d total %d",
                tid, conn->ip, conn->port, conn->fd, QTOTAL(pth->message_queue));
        ret = 0;
    }
    return ret;
}

/* Add connection message */
int procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
    MESSAGE msg = {0};
    int ret = -1;

    if(pth && pth->message_queue && conn)
    {
        msg.msg_id      = MESSAGE_NEW_SESSION;
        msg.fd          = conn->fd;
        msg.handler     = (void *)conn;
        msg.parent      = (void *)pth;
        QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(pth->logger, "Ready for adding msg[%s] connection[%s:%d] via %d total %d", 
                MESSAGE_DESC(MESSAGE_NEW_SESSION), conn->ip, conn->port, 
                conn->fd, QTOTAL(pth->message_queue));
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
        conn->message_queue = pth->message_queue;
        conn->parent = pth;
        if(conn->set(conn) == 0)
        {
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
        ret = pth->service->popconn(pth->service, conn);
        ret |= conn->terminate(conn);
        conn->clean(&conn);
    }
    return ret;
}

/* stop procthread */
void procthread_stop(PROCTHREAD *pth)
{
    MESSAGE msg = {0};

    if(pth && pth->message_queue)
    {
        msg.msg_id      = MESSAGE_STOP;
        msg.parent      = (void *)pth;
        QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(pth->logger, "Ready for stopping procthread[%d]", pth->index);
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
}

/* clean procthread */
void procthread_clean(PROCTHREAD **ppth)
{
    if(*ppth)
    {
        if((*ppth)->service->working_mode != WORKING_PROC)
        {
            QUEUE_CLEAN((*ppth)->message_queue);
        }
        free((*ppth));
        (*ppth) = NULL;
    }
}

/* Initialize procthread */
PROCTHREAD *procthread_init()
{
    PROCTHREAD *pth = NULL;

    if((pth = (PROCTHREAD *)calloc(1, sizeof(PROCTHREAD))))
    {
        pth->message_queue           = QUEUE_INIT();
        pth->run                     = procthread_run;
        pth->addconn                 = procthread_addconn;
        pth->add_connection          = procthread_add_connection;
        pth->newtask                 = procthread_newtask;
        pth->newtransaction          = procthread_newtransaction;
        pth->terminate_connection    = procthread_terminate_connection;
        pth->stop                    = procthread_stop;
        pth->terminate               = procthread_terminate;
        pth->clean                   = procthread_clean;
    }
    return pth;
}
