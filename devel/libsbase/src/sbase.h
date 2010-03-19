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
#define SB_CONN_MAX         65536
#define SB_IP_MAX           16
#define SB_GROUPS_MAX       256
#define SB_NDAEMONS_MAX     64
#define SB_INIT_CONNS       128
#define SB_CHUNKS_MAX       65536
#define SB_BUF_SIZE         65536
#define SB_USEC_SLEEP       1000
#define SB_PROXY_TIMEOUT    20000000
#define SB_HEARTBEAT_INTERVAL 1000
/* service type */
#define S_SERVICE      0x00
#define C_SERVICE      0x01
/* working mode */
#define WORKING_PROC    0x00
#define WORKING_THREAD  0x01
/* connection status */
#define CONN_STATUS_FREE        0x00
#define CONN_STATUS_READY       0x01
#define CONN_STATUS_CONNECTED   0x02
#define CONN_STATUS_WORKING     0x04
#define CONN_STATUS_CLOSED      0x08
/* client running status */
#define C_STATE_FREE            0x00
#define C_STATE_WORKING         0x02
#define C_STATE_USING           0x04
#define C_STATE_OVER            0x08
/* connection running state */
#ifndef S_STATES
#define S_STATE_READY           0x00
#define S_STATE_READ_CHUNK      0x02
#define S_STATE_WRITE_STATE     0x04
#define S_STATE_PACKET_HANDLING 0x08
#define S_STATE_DATA_HANDLING   0x10
#define S_STATE_TASKING         0x20
#define S_STATE_REQ             0x40
#define S_STATE_CLOSE           0x80
#define S_STATES                0xee
#endif
#ifndef D_STATES
#define D_STATE_FREE            0x00
#define D_STATE_RCLOSE          0x02
#define D_STATE_WCLOSE          0x04
#define D_STATE_CLOSE           0x08
#define D_STATES                0x0e
#endif
/* packet type list*/
#define PACKET_CUSTOMIZED       0x01
#define PACKET_CERTAIN_LENGTH   0x02
#define PACKET_DELIMITER        0x04
#define PACKET_PROXY            0x08
#define PACKET_ALL              0x0f
struct _SBASE;
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
struct _CHUNK;
typedef struct _CB_DATA
{
    char *data;
    int ndata;
}CB_DATA;
#define PCB(ptr) ((CB_DATA *)ptr)
typedef struct _SESSION
{
    /* SSL */
    int is_use_SSL;
    void *ctx;

    /* packet */
    int timeout;
    int childid;
    void *child;
    int  parentid;
    void *parent;
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
    int (*timeout_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*transaction_handler)(struct _CONN *, int tid);
}SESSION;

typedef void (CALLBACK)(void *);
typedef struct _SBASE
{
    /* base option */
    int nchilds;
    int connections_limit;
	int usec_sleep;
    int running_status;
	EVBASE *evbase;
    struct _SERVICE **services;
    int running_services;
    long long nheartbeat;

	/* timer && logger */
	void *logger;

    /* evtimer */
    void *evtimer;
    int evid;
    int ssl_id;

    /* message queue for proc mode */
    void *message_queue;

	int  (*set_log)(struct _SBASE *, char *);
	int  (*set_evlog)(struct _SBASE *, char *);
	
	int  (*add_service)(struct _SBASE *, struct _SERVICE *);
    int  (*running)(struct _SBASE *, int time_usec);
	void (*stop)(struct _SBASE *);
	void (*clean)(struct _SBASE **);
}SBASE;
/* Initialize sbase */
int setrlimiter(char *name, int rlimid, int nset);
SBASE *sbase_init();

/* group */
typedef struct _CNGROUP
{
  SESSION session;
  char ip[SB_IP_MAX];
  short port;
  short limit;
  short total;
  short nconns_free;
  int   conns_free[SB_CONN_MAX];
}CNGROUP;

/* service */
typedef struct _SERVICE
{
    /* global */
    int lock;
    int usec_sleep;
    SBASE *sbase;
    void *mutex;

    /* heartbeat */
    /* running heartbeat_handler when looped hearbeat_interval times*/
    int heartbeat_interval;
    void *heartbeat_arg;
    CALLBACK *heartbeat_handler;
    void (*set_heartbeat)(struct _SERVICE *, int interval, CALLBACK *handler, void *arg);
    void (*active_heartbeat)(struct _SERVICE *);

    /* working mode */
    int working_mode;
    struct _PROCTHREAD *daemon;
    struct _PROCTHREAD *iodaemon;
    int nprocthreads;
    struct _PROCTHREAD **procthreads;
    int ndaemons;
    struct _PROCTHREAD **daemons;

    /* socket and inet addr option  */
    int family;
    int sock_type;
    struct  sockaddr_in sa;
    char *ip;
    char *multicast;
    int fd;
    int backlog;
    int port;
    
    /* SSL */
    int is_use_SSL;
    char *cacert_file;
    char *privkey_file;
    void *s_ctx;
    void *c_ctx;

    /* service option */
    int service_type;
    char *service_name;
    int  (*set)(struct _SERVICE *service);
    int  (*run)(struct _SERVICE *service);
    void (*stop)(struct _SERVICE *service);

    /* event option */
    EVBASE *evbase;
    EVENT *event;

    /* message queue for proc mode */
    void *message_queue;

    /* chunks queue */
    //void *chunks_queue;
    int  nqchunks;
    struct _CHUNK *qchunks[SB_CHUNKS_MAX];
    struct _CHUNK *(*popchunk)(struct _SERVICE *service);
    int (*pushchunk)(struct _SERVICE *service, struct _CHUNK *cp);
    CB_DATA *(*newchunk)(struct _SERVICE *service, int len);

    /* connections option */
    int connections_limit; 
    int index_max;
    int running_connections;
    int nconns_free;
    int conns_free[SB_CONN_MAX];
    int nconnection;
    struct _CONN *connections[SB_CONN_MAX];
    int nqconns;
    struct _CONN *qconns[SB_CONN_MAX];

    /* C_SERVICE ONLY */
    int client_connections_limit;
    struct _CONN *(*newproxy)(struct _SERVICE *service, struct _CONN * parent, int inet_family, 
            int sock_type, char *ip, int port, SESSION *session);
    struct _CONN *(*newconn)(struct _SERVICE *service, int inet_family, int sock_type, 
            char *ip, int port, SESSION *session);
    struct _CONN *(*addconn)(struct _SERVICE *service, int sock_type, int fd, 
            char *remote_ip, int remote_port, char *local_ip, int local_port, SESSION *);
    struct _CONN *(*getconn)(struct _SERVICE *service, int groupid);
    int     (*freeconn)(struct _SERVICE *service, struct _CONN *);
    int     (*pushconn)(struct _SERVICE *service, struct _CONN *conn);
    int     (*popconn)(struct _SERVICE *service, struct _CONN *conn);
    struct _CONN *(*findconn)(struct _SERVICE *service, int index);

    /* CAST */
    int (*add_multicast)(struct _SERVICE *service, char *multicast_ip);
    int (*drop_multicast)(struct _SERVICE *service, char *multicast_ip);
    int (*broadcast)(struct _SERVICE *service, char *data, int len);

    /* group */
    CNGROUP groups[SB_GROUPS_MAX];
    int ngroups;
    int (*addgroup)(struct _SERVICE *service, char *ip, int port, int limit, SESSION *session);
    int (*closegroup)(struct _SERVICE *service, int groupid);
    int (*castgroup)(struct _SERVICE *service, char *data, int len);
    int (*stategroup)(struct _SERVICE *service);
    
    /* evtimer */
    void *evtimer;
    int evid;

    /* timer and logger */
    void *logger;
    int  is_inside_logger;
    int (*set_log)(struct _SERVICE *service, char *logfile);

    /* transaction and task */
    int ntask;
    int (*newtask)(struct _SERVICE *, CALLBACK *, void *arg); 
    int (*newtransaction)(struct _SERVICE *, struct _CONN *, int tid);

    /* service default session option */
    SESSION session;
    int (*set_session)(struct _SERVICE *, SESSION *);

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
    int lock;
    int running_status;
	int usec_sleep;
    int index;
    long threadid;

    /* message queue */
    void *ioqmessage;
    void *message_queue;

    /* evbase */
    int have_evbase;
    EVBASE *evbase;

    /* connection */
    struct _CONN **connections;
    int (*addconn)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*add_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*terminate_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);

    /* logger */
    void *logger;

    /* task and transaction */
    int (*newtask)(struct _PROCTHREAD *, CALLBACK *, void *arg); 
    int (*newtransaction)(struct _PROCTHREAD *, struct _CONN *, int tid);

    /* heartbeat */
    void (*active_heartbeat)(struct _PROCTHREAD *,  CALLBACK *handler, void *arg);
    void (*state)(struct _PROCTHREAD *,  CALLBACK *handler, void *arg);

    /* normal */
    void (*run)(void *arg);
    void (*stop)(struct _PROCTHREAD *procthread);
    void (*terminate)(struct _PROCTHREAD *procthread);
    void (*clean)(struct _PROCTHREAD **procthread);
}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init(int have_evbase);

typedef struct _CONN
{
    /* global */
    int index;
    int groupid;
    /* die state */
    int d_state;
    void *parent;

    /* conenction */
    int sock_type;
    int fd;
    char remote_ip[SB_IP_MAX];
    int remote_port;
    char local_ip[SB_IP_MAX];
    int  local_port;
    int (*set)(struct _CONN *);
    int (*close)(struct _CONN *);
    int (*over)(struct _CONN *);
    int (*terminate)(struct _CONN *);

    /* connection bytes stats */
    long long   recv_oob_total;
    long long   sent_oob_total;
    long long   recv_data_total;
    long long   sent_data_total;

    /* evbase */
    EVBASE *evbase;
    EVENT *event;

    /* SSL */
    void *ssl;

    /* evtimer */
    void *evtimer;
    int evid;

    /* buffer */
    void *buffer;
    void *packet;
    void *cache;
    void *chunk;
    void *oob;
    void *exchange;

    /* logger and timer */
    void *logger;

    /* queue */
    void *send_queue;

    /* message queue */
    void *ioqmessage;
    void *message_queue;

    /* client transaction state */
    int status;
    int i_state;
    int c_state;
    int c_id;
    int (*start_cstate)(struct _CONN *);
    int (*over_cstate)(struct _CONN *);

    /* transaction/session */
    int s_id;
    int s_state;

    /* event state */
    int evstate;
#define EVSTATE_INIT   0
#define EVSTATE_WAIT   1 
    int (*wait_evstate)(struct _CONN *);
    int (*over_evstate)(struct _CONN *);

    /* timeout */
    int timeout;
    int (*set_timeout)(struct _CONN *, int timeout_usec);
    int (*timeout_handler)(struct _CONN *);
  
    /* message */
    int (*push_message)(struct _CONN *, int message_id);

    /* session */
    int (*read_handler)(struct _CONN *);
    int (*write_handler)(struct _CONN *);
    int (*packet_reader)(struct _CONN *);
    int (*packet_handler)(struct _CONN *);
    int (*oob_handler)(struct _CONN *);
    int (*data_handler)(struct _CONN *);
    int (*bind_proxy)(struct _CONN *, struct _CONN *);
    int (*proxy_handler)(struct _CONN *);
    int (*close_proxy)(struct _CONN *);
    int (*push_exchange)(struct _CONN *, void *data, int size);
    int (*transaction_handler)(struct _CONN *, int );
    int (*save_cache)(struct _CONN *, void *data, int size);

    /* chunk */
    int (*chunk_reader)(struct _CONN *);
    int (*recv_chunk)(struct _CONN *, int size);
    int (*recv_file)(struct _CONN *, char *file, long long offset, long long size);
    int (*push_chunk)(struct _CONN *, void *data, int size);
    int (*push_file)(struct _CONN *, char *file, long long offset, long long size);
    int (*send_chunk)(struct _CONN *, CB_DATA *chunk, int len);

    /* session option and callback  */
    SESSION session;
    int (*set_session)(struct _CONN *, SESSION *session);
    int (*over_session)(struct _CONN *);
    int (*newtask)(struct _CONN *, CALLBACK *);

    /* normal */
    void (*reset)(struct _CONN *);
    void (*clean)(struct _CONN **pconn);
}CONN, *PCONN;
CONN *conn_init();
#ifdef __cplusplus
 }
#endif
#endif
