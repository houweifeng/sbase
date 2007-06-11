#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "sbase.h"

#ifndef _SERVICE_H
#define _SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

/* Event handler */
void service_event_handler(int, short, void*);
/* Set service  */
int service_set(SERVICE *);
/* Run service */
void service_run(SERVICE *);
/* Add new conn */
int service_addconn(SERVICE *, int , struct sockaddr_in *);
/* Add connection */
void service_addconnection(SERVICE *, CONN *);
/* Terminate service */
void service_terminate(SERVICE *);
/* Clean service */
void service_clean(SERVICE **);

#ifdef __cplusplus
 }
#endif
#endif
