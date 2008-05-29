#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <evbase.h>
#ifndef _SBASE_H
#define _SBASE_H
#ifdef __cplusplus
extern "C" {
#endif
#define SB_CONN_MAX    65536
/* service type */
#define S_SERVICE      0x00
#define C_SERVICE      0x01
/* working mode */
#define WORKING_PROC    0x00
#define WORKING_THREAD  0x01
struct _SBASE;
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
typedef struct _SBASE
{
	int fd;
	int usec_sleep;
	EVBASE *evbase;

	/* timer && logger */
	void *logger;
    void *timer;

	int (*setrlimit)(struct _SBASE *, char *, int , int);
	
	int(*set_log)(struct _SBASE *, char *);
	int(*set_evlog)(struct _SBASE *, char *);
	
	int(*add_service)(struct _SBASE *, struct _SERVICE *);
	int(*running)(struct _SBASE *, int );
	int(*stop)(struct _SBASE *);
}SBASE;
/* Initialize sbase */
SBASE *sbase_init();

/* service */
typedef struct _SERVICE
{
    /* global */
    SBASE *sbase;

    /* working mode */
    int working_mode;
    struct _PROCTHREAD *daemon;
    int nprocthreads;
    struct _PROCTHREAD **procthreads;
    int ndaemons;
    struct _PROCTHREAD **daemons;


    /* socket and inet addr option  */
    int family;
    int sock_type;
    struct  sockaddr_in sa;
    char *ip;
    int port;
    int fd;
    int backlog;

    /* service option */
    int service_type;
    char *service_name;
    int (*set)(struct _SERVICE *service);
    int (*run)(struct _SERVICE *service);
    int (*stop)(struct _SERVICE *service);

    /* event option */
    EVBASE *evbase;
    EVENT *event;

    /* message queue for proc mode */
    void *message_queue;

    /* connections option */
    int connections_limit; 
    int running_connections;
    struct _CONN **connections;
    struct _CONN *(*newconn)(struct _SERVICE *service, int inet_family, int sock_type, 
            char *ip, int port);
    struct _CONN *(*getconn)(struct _SERVICE *service);
    
    /* timer and logger */
    void *timer;
    void *logger;
    int (*set_log)(struct _SERVICE *service, char *logfile);

    /* transaction and task */
    
    /* async dns */

    /* clean */
    void (*clean)(struct _SERVICE **pservice);

}SERVICE;
/* Initialize service */
SERVICE *service_init();
/* procthread */
typedef struct _PROCTHREAD
{
    /* global */
    SERVICE *service;
    int index;

    /* message queue */
    void *message_queue;

    /* evbase */
    EVBASE *evbase;

    /* connection */
    struct _CONN **connections;

    /* logger */
    void *logger;

    /* normal */
    void (*run)(struct _PROCTHREAD *procthread, void *arg);
    void (*stop)(struct _PROCTHREAD *procthread);
}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init();
#ifdef __cplusplus
 }
#endif
#endif
