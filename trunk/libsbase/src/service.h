#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <event.h>
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
void service_addconn(SERVICE *, int , struct sockaddr_in );
/* Terminate service */
void service_terminate(SERVICE *);
/* Clean service */
void service_clean(SERVICE **);

#endif
#ifdef __cplusplus
 }
#endif
#endif
