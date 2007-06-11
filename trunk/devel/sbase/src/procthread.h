#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "conn.h"
#include "queue.h"
#include "queue.h"
#include "timer.h"
#include "sbase.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


#ifndef _PROCTHREAD_H
#define _PROCTHREAD_H
#ifdef __cplusplus
extern "C" {
#endif
/* Running procthread */
void procthread_run(void *);

/* Add connection message */
void procthread_addconn(PROCTHREAD *, CONN *);

/* Add new connection */
void procthread_add_connection(PROCTHREAD *, CONN *);

/* Terminate connection */
void procthread_terminate_connection(PROCTHREAD *, CONN *);

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *);

/* Clean procthread */
void procthread_clean(PROCTHREAD **);

#ifdef __cplusplus
 }
#endif
#endif
