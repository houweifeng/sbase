#include "sbase.h"
#ifndef _PROCTHREAD_H
#define _PROCTHREAD_H
/* run procthread */
void procthread_run(void *arg);
/* add new task */
int procthread_newtask(PROCTHREAD *pth, CALLBACK *, void *arg);

/* add new transaction */
int procthread_newtransaction(PROCTHREAD *pth, CONN *, int tid);

/* Add connection message */
int procthread_addconn(PROCTHREAD *, CONN *);

/* Add new connection */
int procthread_add_connection(PROCTHREAD *, CONN *);

/* Terminate connection */
int procthread_terminate_connection(PROCTHREAD *, CONN *);

/* Add stop message on procthread */
void procthread_stop(PROCTHREAD *);

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *);

/* Clean procthread */
void procthread_clean(PROCTHREAD **);
#endif
