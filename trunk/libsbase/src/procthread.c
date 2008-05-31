#include "procthread.h"
#include "queue.h"
#include "logger.h"
#include "message.h"

/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *procthread = (PROCTHREAD *)arg;

    if(procthread)
    {
    }
#ifdef HAVE_PTHREAD
    pthread_exit();
#endif
}

/* add new task */
int procthread_newtask(PROCTHREAD *pth, CALLBACK *task_handler, void *arg)
{
}

/* add new transaction */
int procthread_newtransaction(PROCTHREAD *pth, CONN *conn, int tid)
{
}

/* Add connection message */
int procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
    MESSAGE msg = {0};
    if(pth && conn)
    {
        msg.fd          = MESSAGE_NEW_SESSION;
        msg.fd          = conn->fd;
        msg.handler     = (void *)conn;
        msg.parent      = (void *)pth;
        QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(pth->logger, "Ready for adding msg[%s] connection[%s:%d] via %d", 
                MESSAGE_DESC(MESSAGE_NEW_SESSION), conn->ip, conn->port, conn->fd);
    }
}

/* Add new connection */
int procthread_add_connection(PROCTHREAD *pth, CONN *conn)
{
}

/* Terminate connection */
int procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
}

/* clean procthread */
void procthread_clean(PROCTHREAD **ppth)
{
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
        pth->terminate_connection    = procthread_terminate_connection;
        pth->terminate               = procthread_terminate;
        pth->clean                   = procthread_clean;
    }
}
