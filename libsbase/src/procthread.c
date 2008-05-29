#include "procthread.h"
#include "queue.h"

/* run procthread */
void procthread_run(void *arg)
{
    PROCTHREAD *procthread = (PROCTHREAD *)arg;

    if(procthread)
    {
    }
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
