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
#define SB_CONN_MAX             65536
#define SB_GROUP_CONN_MAX       1024
#define SB_IP_MAX               16
#define SB_XIDS_MAX             16
#define SB_GROUPS_MAX           256
#define SB_SERVICE_MAX          256
#define SB_THREADS_MAX          256
#define SB_INIT_CONNS           256
#define SB_QCONN_MAX            256
#define SB_CHUNKS_MAX           256
#define SB_BUF_SIZE             65536
#define SB_USEC_SLEEP           1000
#define SB_PROXY_TIMEOUT        20000000
#define SB_HEARTBEAT_INTERVAL   11000
/* service type */
#define S_SERVICE               0x00
#define C_SERVICE               0x01
/* working mode */
#define WORKING_PROC            0x00
#define WORKING_THREAD          0x01
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
/* ERROR wait state */
#define E_STATE_OFF             0x00
#define E_STATE_ON              0x01
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
#ifndef __TYPEDEF__MUTEX
#define __TYPEDEF__MUTEX
#ifdef HAVE_SEMAPHORE
#include <semaphore.h>
typedef struct _MUTEX
{
        sem_t sem;
}MUTEX;
#else
#include <pthread.h>
typedef struct _MUTEX
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int nowait;
}MUTEX;
#endif
#endif
#ifndef __TYPEDEF__MMBLOCK
#define __TYPEDEF__MMBLOCK
typedef struct _MMBLOCK
{
    char *data;
    int  ndata;
    int  size;
    int  left;
    int  bit;
    char *end;
}MMBLOCK;
#endif
#ifndef __TYPEDEF__CHUNK
#define __TYPEDEF__CHUNK
#define CHUNK_FILE_NAME_MAX     1024
typedef struct _CHUNK
{
    char *data;
    int  ndata;
    int  status;
    int  bsize;
    int  type;
    int  fd;
    int  bits;
    off_t size;
    off_t offset;
    off_t left;
    off_t mmleft;
    off_t mmoff;
    char *mmap;
    char *end;
    char filename[CHUNK_FILE_NAME_MAX];
}CHUNK;
#endif
typedef struct _CB_DATA
{
    char *data;
    int ndata;
    int size;
}CB_DATA;
#define PCB(mmm) ((CB_DATA *)((void *)&mmm))
typedef struct _SESSION
{
    /* SSL/timeout */
    int  flag;
    int  is_use_SSL;
    int  timeout;
    int  childid;
    int  parentid;
    int  packet_type;
    int  packet_length;
    int  packet_delimiter_length;
    int  buffer_size;
    int  id;
    void *child;
    void *ctx;
    void *parent;
    char *packet_delimiter;

    /* methods */
    int (*error_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*packet_reader)(struct _CONN *, CB_DATA *buffer);
    int (*packet_handler)(struct _CONN *, CB_DATA *packet);
    int (*data_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*file_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache);
    int (*oob_handler)(struct _CONN *, CB_DATA *oob);
    int (*timeout_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    int (*transaction_handler)(struct _CONN *, int tid);
    int (*ok_handler)(struct _CONN *);
}SESSION;

typedef void (CALLBACK)(void *);
typedef struct _SBASE
{
    /* base option */
    int nchilds;
    int connections_limit;
	int usec_sleep;
    int running_status;
    int running_services;
    int  evlog_level;
    /* evtimer */
    int evid;
    int ssl_id;
    long long nheartbeat;

	/* timer && logger */
	void *logger;
	EVBASE *evbase;
    char *evlogfile;
    void *evtimer;
    struct _SERVICE *services[SB_SERVICE_MAX];

    /* message queue for proc mode */
    void *message_queue;

	int  (*set_log)(struct _SBASE *, char *);
    int  (*set_log_level)(struct _SBASE *sbase, int level);
	int  (*set_evlog)(struct _SBASE *, char *);
	int  (*set_evlog_level)(struct _SBASE *, int level);
	
	int  (*add_service)(struct _SBASE *, struct _SERVICE *);
	void (*remove_service)(struct _SBASE *, struct _SERVICE *);
    int  (*running)(struct _SBASE *, int time_usec);
	void (*stop)(struct _SBASE *);
	void (*clean)(struct _SBASE *);
}SBASE;
/* Initialize sbase */
int setrlimiter(char *name, int rlimid, int nset);
SBASE *sbase_init();

/* group */
typedef struct _CNGROUP
{
  short status;
  short nconnected;
  short port;
  short limit;
  int   total;
  int   nconns_free;
  int   conns_free[SB_GROUP_CONN_MAX];
  char  ip[SB_IP_MAX];
  SESSION session;
}CNGROUP;

/* service */
typedef struct _SERVICE
{
    /* global */
    int id;
    int cond;
    int lock;
    int usec_sleep;
    int use_cond_wait;
    int init_conns;
    int nconn;
    int nnewconns;
    int nfreeconns;
    int nchunks;
    int connections_limit; 
    int index_max;
    int running_connections;
    int nconns_free;
    int nconnection;
    int client_connections_limit;
    int nqchunks;
    int service_type;
    int fd;
    int backlog;
    int port;
    int sock_type;
    int family;
    int heartbeat_interval;
    int working_mode;
    int use_iodaemon;
    int nprocthreads;
    int ndaemons;
    int is_use_SSL;
    int nqconns;
    int ngroups;
    int evid;
    int is_inside_logger;
    int ntask;
    struct  sockaddr_in sa;
    EVENT event;
    MUTEX mutex;

    int conns_free[SB_CONN_MAX];
    /* mutex */
    SBASE *sbase;

    /* heartbeat */
    void *heartbeat_arg;
    CALLBACK *heartbeat_handler;
    void (*set_heartbeat)(struct _SERVICE *, int interval, CALLBACK *handler, void *arg);
    void (*active_heartbeat)(struct _SERVICE *);

    /* working mode */
    struct _PROCTHREAD *daemon;
    struct _PROCTHREAD *iodaemon;
    struct _PROCTHREAD **procthreads;
    struct _PROCTHREAD **daemons;

    /* socket and inet addr option  */
    char *ip;
    char *multicast;
        
    /* SSL */
    char *cacert_file;
    char *privkey_file;
    void *s_ctx;
    void *c_ctx;

    /* service option */
    char *service_name;
    int  (*set)(struct _SERVICE *service);
    int  (*run)(struct _SERVICE *service);
    void (*stop)(struct _SERVICE *service);

    /* event option */
    EVBASE *evbase;

    /* message queue for proc mode */
    void *message_queue;
    void *send_queue;

    //void *chunks_queue;
    CHUNK *qchunks[SB_CHUNKS_MAX];
    CHUNK *(*popchunk)(struct _SERVICE *service);
    int (*pushchunk)(struct _SERVICE *service, CHUNK *cp);
    CB_DATA *(*newchunk)(struct _SERVICE *service, int len);

    /* connections option */
    struct _CONN *connections[SB_CONN_MAX];
    struct _CONN *qconns[SB_CONN_MAX];

    /* C_SERVICE ONLY */
    struct _CONN *(*newproxy)(struct _SERVICE *service, struct _CONN * parent, int inet_family, 
            int sock_type, char *ip, int port, SESSION *session);
    struct _CONN *(*newconn)(struct _SERVICE *service, int inet_family, int sock_type, 
            char *ip, int port, SESSION *session);
    struct _CONN *(*addconn)(struct _SERVICE *service, int sock_type, int fd, 
            char *remote_ip, int remote_port, char *local_ip, int local_port, 
            SESSION *, int status);
    struct _CONN *(*getconn)(struct _SERVICE *service, int groupid);
    int     (*freeconn)(struct _SERVICE *service, struct _CONN *);
    int     (*pushconn)(struct _SERVICE *service, struct _CONN *conn);
    int     (*okconn)(struct _SERVICE *service, struct _CONN *conn);
    int     (*popconn)(struct _SERVICE *service, struct _CONN *conn);
    struct _CONN *(*findconn)(struct _SERVICE *service, int index);
    void    (*overconn)(struct _SERVICE *service, struct _CONN *conn);

    /* CAST */
    int (*add_multicast)(struct _SERVICE *service, char *multicast_ip);
    int (*drop_multicast)(struct _SERVICE *service, char *multicast_ip);
    int (*broadcast)(struct _SERVICE *service, char *data, int len);

    /* group */
    int (*addgroup)(struct _SERVICE *service, char *ip, int port, int limit, SESSION *session);
    int (*closegroup)(struct _SERVICE *service, int groupid);
    int (*castgroup)(struct _SERVICE *service, char *data, int len);
    int (*stategroup)(struct _SERVICE *service);
    
    /* evtimer */
    void *etimer;
    void *evtimer;
    
    /* timer and logger */
    void *logger;
    int (*set_log)(struct _SERVICE *service, char *logfile);
    int (*set_log_level)(struct _SERVICE *service, int level);

    /* transaction and task */
    int (*newtask)(struct _SERVICE *, CALLBACK *, void *arg); 
    int (*newtransaction)(struct _SERVICE *, struct _CONN *, int tid);

    /* service default session option */
    int (*set_session)(struct _SERVICE *, SESSION *);

    /* clean */
    void (*clean)(struct _SERVICE *pservice);
    void (*close)(struct _SERVICE *);
    SESSION session;
    CNGROUP groups[SB_GROUPS_MAX];
}SERVICE;
/* Initialize service */
SERVICE *service_init();
/* procthread */
typedef struct _PROCTHREAD
{
    /* global */
    int status; 
    int lock;
    int running_status;
	int usec_sleep;
    int index;
    int cond;
    int use_cond_wait;
    int have_evbase;
    long threadid;
    MUTEX mutex;
    EVENT event;
    void *evtimer;
    SERVICE *service;

    /* message queue */
    void *iodaemon;
    void *ioqmessage;
    void *message_queue;
    void *send_queue;

    /* evbase */
    EVBASE *evbase;

    /* connection */
    struct _CONN **connections;
    int (*pushconn)(struct _PROCTHREAD *procthread, int fd, void *ssl);
    int (*newconn)(struct _PROCTHREAD *procthread, int fd, void *ssl);
    int (*addconn)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*add_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*shut_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    int (*over_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
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
    void (*wakeup)(struct _PROCTHREAD *procthread);
    void (*stop)(struct _PROCTHREAD *procthread);
    void (*terminate)(struct _PROCTHREAD *procthread);
    void (*clean)(struct _PROCTHREAD *procthread);
}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init(int have_evbase);
/* CONN */
typedef struct _CONN
{
    /* global */
    int index;
    int groupid;
    int gindex;
    int s_id;
    int s_state;
    int timeout;
    int evstate;
    int status;
    int i_state;
    int c_state;
    int e_state;
    int c_id;
    int d_state;
    int evid;
    int qid;
    int bits;
    /* conenction */
    int  sock_type;
    int  fd;
    int  remote_port;
    int  local_port;
    /* xid */
    int xids[SB_XIDS_MAX];
    EVENT event;
    MUTEX mutex;
    /* evbase */
    EVBASE *evbase;
    void *parent;
    /* SSL */
    void *ssl;
    /* evtimer */
    void *evtimer;
    /* buffer */
    MMBLOCK buffer;
    MMBLOCK packet;
    MMBLOCK cache;
    MMBLOCK oob;
    MMBLOCK exchange;
    CHUNK chunk;
    /* logger and timer */
    void *logger;
    /* queue */
    void *send_queue;
    /* message queue */
    void *iodaemon;
    void *ioqmessage;
    void *message_queue;
    int (*set)(struct _CONN *);
    int (*close)(struct _CONN *);
    int (*over)(struct _CONN *);
    int (*terminate)(struct _CONN *);

    /* client transaction state */
    int (*start_cstate)(struct _CONN *);
    int (*over_cstate)(struct _CONN *);
    /* error state */
    int (*wait_estate)(struct _CONN *);
    int (*over_estate)(struct _CONN *);
    
    /* event state */
#define EVSTATE_INIT   0
#define EVSTATE_WAIT   1 
    int (*wait_evstate)(struct _CONN *);
    int (*over_evstate)(struct _CONN *);

    /* timeout */
    int (*set_timeout)(struct _CONN *, int timeout_usec);
    int (*timeout_handler)(struct _CONN *);
    int (*over_timeout)(struct _CONN *);
  
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
    int (*over_chunk)(struct _CONN *);
    
    /* normal */
    void (*reset_xids)(struct _CONN *);
    void (*reset_state)(struct _CONN *);
    void (*reset)(struct _CONN *);
    void (*clean)(struct _CONN *conn);
    /* session option and callback  */
    int (*set_session)(struct _CONN *, SESSION *session);
    int (*over_session)(struct _CONN *);
    int (*newtask)(struct _CONN *, CALLBACK *);
    int (*get_service_id)(struct _CONN *);
    /* connection bytes stats */
    long long   recv_oob_total;
    long long   sent_oob_total;
    long long   recv_data_total;
    long long   sent_data_total;
    /* xid 64 bit */
    int64_t xids64[SB_XIDS_MAX];
    SESSION session;
    char remote_ip[SB_IP_MAX];
    char local_ip[SB_IP_MAX];
}CONN, *PCONN;
CONN *conn_init();
#ifdef __cplusplus
 }
#endif
#endif
