#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "conn.h"
#include "queue.h"
#include "queue.h"
#include "timer.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif


#ifndef _PROCTHREAD_H
#define _PROCTHREAD_H
#ifdef __cplusplus
extern "C" {
#endif
/* Running procthread */
void procthread_run(PROCTHREAD *);

/* Add connection message */
void procthread_addconn(PROCTHREAD *, int , struct sockaddr_in);

/* Add new connection */
void procthread_add_connection(PROCTHREAD *);

/* Terminate connection */
void procthread_terminate_connection(PROCTHREAD *);

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *);

/* Clean procthread */
void procthread_clean(PROCTHREAD **);

#endif
#ifdef __cplusplus
 }
#endif
#endif
