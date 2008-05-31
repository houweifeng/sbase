#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
#define SB_IP_MAX      16
/* service type */
#define S_SERVICE      0x00
#define C_SERVICE      0x01
/* working mode */
#define WORKING_PROC    0x00
#define WORKING_THREAD  0x01
/* connection status */
#define CONN_STATUS_READY       1
#define CONN_STATUS_CONNECTED   2
#define CONN_STATUS_CLOSED      4
/* connection running state */
#ifndef S_STATES
#define S_STATE_READY           0x00
#define S_STATE_READ_CHUNK      0x02
#define S_STATE_WRITE_STATE     0x04
#define S_STATE_PACKET_HANDLING 0x08
#define S_STATE_DATA_HANDLING   0x10
#define S_STATE_REQ             0x12
#define S_STATE_CLOSE           0x64
#define S_STATES    (S_STATE_READY & S_STATE_READ_CHUNK & S_STATE_WRITE_STATE \
        S_STATE_PACKET_HANDLING & S_STATE_DATA_HANDLING & S_STATE_REQ & S_STATE_CLOSE )
#endif
struct _SBASE;
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
typedef struct _CB_DATA
{
    char *data;
    int ndata;
}CB_DATA;
#define PCB(ptr) ((CB_DATA *)ptr)
typedef struct _SESSION
{
    /* packet */
    int  packet_type;
    int  packet_length;
    char *packet_delimiter;
    int  packet_delimiter_length;
    int  buffer_size;

    /* methods */
    int (*error_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*packet_reader)(struct _CONN *, CB_DATA *buffer);
    int (*packet_handler)(struct _CONN *, CB_DATA *packet);
    int (*data_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*file_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache);
    int (*oob_handler)(struct _CONN *, CB_DATA *oob);
}SESSION;

typedef void (*CALLBACK)(void *);
typedef struct _SBASE
{
    /* base option */
	int usec_sleep;
    int running_status;
	EVBASE *evbase;
    struct _SERVICE **services;
    int running_service;

	/* timer && logger */
	void *logger;
    void *timer;

	int (*setrlimit)(struct _SBASE *, char *, int , int);
	
	int(*set_log)(struct _SBASE *, char *);
	int(*set_evlog)(struct _SBASE *, char *);
	
	int(*add_service)(struct _SBASE *, struct _SERVICE *);
	int(*running)(struct _SBASE *, int );
	int(*stop)(struct _SBASE *);
	void(*clean)(struct _SBASE **);
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
            char *ip, int port, SESSION *session);
    struct _CONN *(*addconn)(struct _SERVICE *service, int fd, char *ip, int port, SESSION *);
    struct _CONN *(*getconn)(struct _SERVICE *service);
    struct _CONN *(*pushconn)(struct _SERVICE *service, struct _CONN *conn);
    struct _CONN *(*popconn)(struct _SERVICE *service, int);
    
    /* timer and logger */
    void *timer;
    void *logger;
    int (*set_log)(struct _SERVICE *service, char *logfile);

    /* transaction and task */
    
    /* async dns */

    /* service default session option */
    SESSION session;

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
    int (*addconn)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*add_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*terminate_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);

    /* logger */
    void *logger;

    /* normal */
    void (*run)(void *arg);
    void (*terminate)(struct _PROCTHREAD *procthread);
    void (*clean)(struct _PROCTHREAD **procthread);
}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init();

typedef struct _CONN
{
    /* global */
    int index;
    void *parent;

    /* conenction */
    int fd;
    char ip[SB_IP_MAX];
    int  port;
    int (*set)(struct _CONN *);
    int (*close)(struct _CONN *);
    int (*terminate)(struct _CONN *);

    /* connection bytes stats */
    long long   recv_oob_total;
    long long   sent_oob_total;
    long long   recv_data_total;
    long long   sent_data_total;

    /* evbase */
    EVBASE *evbase;
    EVENT *event;

    /* buffer */
    void *buffer;
    void *packet;
    void *cache;
    void *chunk;
    void *oob;

    /* logger and timer */
    void *logger;
    void *timer;

    /* queue */
    void *send_queue;

    /* message queue */
    void *message_queue;

    /* client transaction state */
    int c_state;
    int c_id;
    int (*start_cstate)(struct _CONN *);
    int (*over_cstate)(struct _CONN *);

    /* transaction */
    int s_id;
    int s_state;

    /* timeout */
    int timeout;
    int (*set_timeout)(struct _CONN *, int timeout_usec);
  
    /* message */
    int (*push_message)(struct _CONN *, int message_id);

    /* session */
    int (*read_handler)(struct _CONN *);
    int (*write_handler)(struct _CONN *);
    int (*packet_reader)(struct _CONN *);
    int (*packet_handler)(struct _CONN *);
    int (*oob_handler)(struct _CONN *);
    int (*data_handler)(struct _CONN *);
    int (*save_cache)(struct _CONN *, void *data, int size);

    /* chunk */
    int (*chunk_reader)(struct _CONN *);
    int (*recv_chunk)(struct _CONN *, int size);
    int (*recv_file)(struct _CONN *, char *file, long long offset, long long size);
    int (*push_chunk)(struct _CONN *, void *data, int size);
    int (*push_file)(struct _CONN *, char *file, long long offset, long long size);

    /* session option and callback  */
    SESSION session;
    int (*set_session)(struct _CONN *, SESSION *session);

    /* normal */
    void (*clean)(struct _CONN **pconn);
}CONN;
CONN *conn_init(int fd, char *ip, int port);
#ifdef __cplusplus
 }
#endif
#endif
