#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <evbase.h>
#ifndef _SBASE_H
#define _SBASE_H
#ifdef __cplusplus
extern "C" {
#endif

/* INET */
#ifndef TCP_T
#define TCP_T SOCK_STREAM
#endif
#ifndef UDP_T	
#define UDP_T SOCK_DGRAM 
#endif
#ifndef SOCK_MIX      
#define SOCK_MIX (SOCK_DGRAM | SOCK_STREAM)
#endif
#ifndef DGRAM_SIZE 	
#define DGRAM_SIZE 1024 
#endif
#ifndef SB_BUF_SIZE
    /* 1M = 0x100000 */
#define	SB_BUF_SIZE   0x100000 
#endif
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 	65535
#endif
#ifndef MAX_PACKET_LEN
#define MAX_PACKET_LEN		65535
#endif
#ifndef MAX_PROCTHREADS
#define MAX_PROCTHREADS		2048
#endif
#ifndef MIN_SLEEP_USEC
#define MIN_SLEEP_USEC		1000
#endif 
#ifndef MIN_HEARTBEAT_INTERVAL
#define MIN_HEARTBEAT_INTERVAL  1
#endif
#ifndef MIN_CONN_TIMEOUT
#define MIN_CONN_TIMEOUT	120
#endif
#ifndef	INET_BACKLOG
#define	INET_BACKLOG		65535 
#endif
#ifndef PACKET_ALL
#define PACKET_CUSTOMIZED     	0x01 /* packet customized by user */
#define PACKET_CERTAIN_LENGTH	0x02 /* packet with certain length */
#define PACKET_DELIMITER	0x04 /* packet spliting with delimiter */
#define PACKET_ALL        (PACKET_CUSTOMIZED | PACKET_CERTAIN_LENGTH | PACKET_DELIMITER)
#endif
#ifndef S_SERVICE
#define S_SERVICE 0
#define C_SERVICE 1
#endif

/* TYPES PREDEFINE */
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
struct _BUFFER;
struct _CHUNK;

#ifndef _TYPEDEF_BUFFER
#define _TYPEDEF_BUFFER
typedef struct _BUFFER
{
        void *data;
        void *end;
        size_t size;
        void *mutex;

        void 	*(*calloc)(struct _BUFFER *, size_t);
        void 	*(*malloc)(struct _BUFFER *, size_t);
        void 	*(*recalloc)(struct _BUFFER *, size_t);
        void 	*(*remalloc)(struct _BUFFER *, size_t);
        int 	(*push)(struct _BUFFER *, void *, size_t);
        int 	(*del)(struct _BUFFER *, size_t);
        void 	(*reset)(struct _BUFFER *);
        void 	(*clean)(struct _BUFFER **);
}BUFFER;

#define BUFFER_VIEW(buf) \
    { \
        if(buf) \
        { \
            fprintf(stdout, "buf:%08X\n" \
                    "buf->data:%08X\n" \
                    "buf->end:%08X\n" \
                    "buf->size:%ld\n" \
                    "buf->recalloc():%08X\n" \
                    "buf->remalloc():%08X\n" \
                    "buf->push():%08X\n" \
                    "buf->del():%08X\n" \
                    "buf->reset():%08X\n" \
                    "buf->clean():%08X\n", \
                    buf, buf->data, buf->end, buf->size, \
                    buf->recalloc, buf->remalloc, \
                    buf->push, buf->del, \
                    buf->reset, buf->clean); \
        } \
    }
struct _BUFFER *buffer_init();
#endif

    /* CHUNK */
#ifndef _TYPEDEF_CHUNK
#define _TYPEDEF_CHUNK
#define MEM_CHUNK   0x02
#define FILE_CHUNK  0x04
#define ALL_CHUNK  (MEM_CHUNK | FILE_CHUNK)
#define FILE_NAME_LIMIT 255
typedef struct _CHUNK{
        /* property */
        int id;
        int type;
        struct _BUFFER *buf;
        struct {
            int   fd ;
            char  name[FILE_NAME_LIMIT + 1];
        } file;
        long long  offset;
        long long  len;
        void *mutex;

        /* method */
        int (*set)(struct _CHUNK *, int , int , char *, long long , long long );
        int (*append)(struct _CHUNK *, void *, size_t); 
        int (*fill)(struct _CHUNK *, void *, size_t); 
        int (*send)(struct _CHUNK *, int , size_t );
        void (*reset)(struct _CHUNK *); 
        void (*clean)(struct _CHUNK **); 
}CHUNK;
#define CHUNK_SIZE sizeof(CHUNK)
/* Initialize struct CHUNK */
struct _CHUNK *chunk_init();
#define CHUNK_VIEW(chunk) \
    { \
        if(chunk) \
        { \
            fprintf(stdout, "chunk:%08X\n" \
                    "chunk->id:%d\n" \
                    "chunk->type:%02X\n" \
                    "chunk->buf:%08X\n" \
                    "chunk->buf->data:%08X\n" \
                    "chunk->buf->size:%u\n" \
                    "chunk->file.fd:%d\n" \
                    "chunk->file.name:%s\n" \
                    "chunk->offset:%lld\n" \
                    "chunk->len:%lld\n\n", \
                    chunk, chunk->id, chunk->type, \
                    chunk->buf, chunk->buf->data, chunk->buf->size, \
                    chunk->file.fd, chunk->file.name, \
                    chunk->offset, chunk->len \
                   ); \
        } \
    }
#endif

/* Transaction state */
#ifndef S_STATES
#define S_STATE_READY		    0x00
#define S_STATE_READ_CHUNK	    0x02
#define S_STATE_WRITE_STATE     0x04
#define S_STATE_PACKET_HANDLING	0x08
#define S_STATE_DATA_HANDLING	0x10
#define S_STATE_REQ             0x12
#define S_STATE_CLOSE		    0x64
#define S_STATES	(S_STATE_READY & S_STATE_READ_CHUNK & S_STATE_WRITE_STATE \
        S_STATE_PACKET_HANDLING & S_STATE_DATA_HANDLING & S_STATE_REQ & S_STATE_CLOSE )
#endif

/* WORK TYPE */
#define WORKING_PROC	0x00
#define WORKING_THREAD	0x01

#ifndef _TYPEDEF_SBASE
#define _TYPEDEF_SBASE
typedef struct _SBASE{
        /* PROCTHREAD */
        int 	working_mode ;
        int	max_procthreads;

        /* Service options */
        struct _SERVICE **services;
        int	running_services;

        /* Event options */
        EVBASE *evbase;

        /* Message Queue options for WORKING_PROC_MODE */
        void  *message_queue;

        /* Running options */
        int running_status;
        uint32_t sleep_usec;

        /* Global options */
        void *timer;
        void *logger;
        void *evlogger;

        /* APIS */
        int  (*add_service)(struct _SBASE * , struct _SERVICE *);
        int  (*set_log)(struct _SBASE * , char *);
        int  (*set_evlog)(struct _SBASE * , char *);
        int  (*setrlimit)(struct _SBASE * , char *, int, int);
        int  (*running)(struct _SBASE *, uint32_t seconds);
        void (*running_once)(struct _SBASE *);
        int  (*stop)(struct _SBASE * );
        void (*clean)(struct _SBASE **);
    }SBASE;
    /* Initialize struct sbase */
    SBASE *sbase_init();
#endif

    /* SERVICE */
#ifndef _TYPEDEF_SERVICE
#define _TYPEDEF_SERVICE
typedef struct _SERVICE
{
        /* INET options */
        int socket_type;
        int family;
        int fd;
        char *name ;
        char *ip;
        int port;
        struct sockaddr_in sa;
        int backlog ;

        /* Service option */
        int service_type;

        /* Event options */
        EVBASE *evbase;
        EVENT  *event;

        /* Message Queue options for WORKING_PROC_MODE */
        void  *message_queue;

        /* Prothread options */
        int working_mode;
        int max_procthreads;
        int running_procthreads;
        struct _PROCTHREAD **procthreads;
        struct _PROCTHREAD *procthread;

        /* Connection options */
        int max_connections;
        int running_connections;
        int running_max_fd;
        struct _CONN **connections;
        /* Client options ONLY */
        int connections_limit;

        /* Packet options */
        int packet_type;
        int packet_length;
        char *packet_delimiter;
        int  packet_delimiter_length;
        uint32_t buffer_size;

        /* Global options */
        void *timer;
        void *logger;
        char *logfile;
        void *evlogger;
        char *evlogfile;

        /* Running options */
        int      running_status;
		long long nheartbeat;
        uint32_t heartbeat_interval;
        void     *cb_heartbeat_arg;
        uint32_t sleep_usec;
        long long conn_timeout;

		/* mutext lock */
		void *mutex;

        /**************** Callback options for service *****************/
        /* Heartbeat Handler */
        void (*cb_heartbeat_handler)(void *arg);
        /* error handler */
        void (*cb_error_handler)(struct _CONN*);
        /* Read From Buffer and return packet length to get */
        int (*cb_packet_reader)(struct _CONN*, struct _BUFFER *);
        /* Packet Handler */
        void (*cb_packet_handler)(struct _CONN*, struct _BUFFER *);
        /* Data Handler */
        void (*cb_data_handler)(struct _CONN*, struct _BUFFER *,
                struct _CHUNK *, struct _BUFFER *);
        /* OOB Data Handler */
        void (*cb_oob_handler)(struct _CONN *, struct _BUFFER *oob);
        /* Transaction handler */
        void (*cb_transaction_handler)(struct _CONN *, int tid);

        /* Methods */
        void (*event_handler)(int, short, void*);
        int  (*set)(struct _SERVICE * );
        void (*run)(struct _SERVICE * );
        struct _CONN* (*addconn)(struct _SERVICE *, int , struct sockaddr_in *);
        void (*active_heartbeat)(struct _SERVICE *);
        /******** Client methods *************/
        void (*newtransaction)(struct _SERVICE *, struct _CONN *, int tid);
        void (*state_conns)(struct _SERVICE *);
        struct _CONN *(*newconn)(struct _SERVICE *, char *, int);
        struct _CONN *(*getconn)(struct _SERVICE *);
		void (*pushconn)(struct _SERVICE *, struct _CONN *);
		void (*popconn)(struct _SERVICE *, int );
        /** terminate methods **/
        void (*terminate)(struct _SERVICE * );
        void (*clean)(struct _SERVICE ** );
}SERVICE;
/* Initialize service */
SERVICE *service_init();
#endif

    /* PROCTHREAD */
#ifndef _TYPEDEF_PROCTHREAD
#define _TYPEDEF_PROCTHREAD
typedef struct _PROCTHREAD
{
        /* PROCTHREAD ID INDEX */
        int id;
        int index;
        struct _SERVICE *service;

        /* Event options */
        EVBASE			*evbase;

        /* Running options */
        int                     running_status;
        struct _CONN            **connections;
        uint32_t                sleep_usec;

        /* MESSAGE QUEUE OPTIONS */
        void 		*message_queue;

        /* Timer options */
        void 		*timer;

        /* Logger option */
        void 		*logger;

        /* Methods */
        void (*run)(void *);
        void (*running_once)(struct _PROCTHREAD *);
        void (*addconn)(struct _PROCTHREAD *, struct _CONN *);
        void (*newtransaction)(struct _PROCTHREAD *, struct _CONN *, int tid);
        void (*add_connection)(struct _PROCTHREAD *, struct _CONN *);
        void (*terminate_connection)(struct _PROCTHREAD *, struct _CONN *);
        void (*terminate)(struct _PROCTHREAD*);
        void (*clean)(struct _PROCTHREAD**);

}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init();
#endif


/* CONNECTION */
#ifndef _TYPEDEF_CONN
#define _TYPEDEF_CONN
#define C_STATE_FREE      0x00
#define C_STATE_USING     0x02
#define C_STATE_CLOSE     0x04
#define IP_MAX            16
#define CONN_NONBLOCK     1
#define CONN_SLEEP_USEC	  100u
#define CONN_IO_TIMEOUT   100000u
#define RECONNECT_MAX     10000
#ifndef CONN_MAX
#define CONN_MAX          65535
#endif
/* struct CONN */
typedef struct _CONN
{
        /* INET options */
        char		ip[IP_MAX];
        int			port;
        struct		sockaddr_in sa;
        int			fd;
        /* Transaction option */
        int			s_id;
        int			s_state;
        long long	timeout;
        /* Client options */
        int         c_id;
        int         c_state;
        int         index;

        /* Packet options */
        int			packet_type;
        int 		packet_length;
        char 		*packet_delimiter;
        int  		packet_delimiter_length;
        uint32_t 	buffer_size;
        /* connection bytes stats */
        long long 	recv_oob_total;
        long long 	sent_oob_total;
        long long 	recv_data_total;
        long long 	sent_data_total;

        /* Global  options */
        //parent pointer 
        void			*parent;
        //message queue 
        void        *message_queue;
        //buffer 
        struct _BUFFER		*buffer;
        //OOB 
        struct _BUFFER      *oob;
        //cache
        struct _BUFFER      *cache;
        //packet 
        struct _BUFFER		*packet;
        // chunk 
        struct _CHUNK		*chunk;
        // send queue 
        void 		*send_queue;
        // Logger
        void       *logger;
        // Timer
        void        *timer;

        /* Event options */
        EVBASE 			*evbase;
        EVENT			*event;

        /* Callback */
        struct _CONN *callback_conn;

        /* error handler */
        void (*cb_error_handler)(struct _CONN *);
        /* Read From Buffer and return packet length to get */
        int (*cb_packet_reader)(struct _CONN*, struct _BUFFER *);
        /* Packet Handler */
        void (*cb_packet_handler)(struct _CONN*, struct _BUFFER *);
        /* Data Handler */
        void (*cb_data_handler)(struct _CONN*, struct _BUFFER *,
                struct _CHUNK *, struct _BUFFER *);
        /* OOB Data Handler */
        void (*cb_oob_handler)(struct _CONN *, struct _BUFFER *oob);
        /* Transaction handler */
        void (*cb_transaction_handler)(struct _CONN *, int tid);

        /* Methods */
        int	      (*set)(struct _CONN *);
        void	  (*event_handler)(int, short, void *);
        void      (*state_handler)(struct _CONN *);
        void      (*read_handler)(struct _CONN *);
        void      (*write_handler)(struct _CONN *);
        int    	  (*packet_reader)(struct _CONN *);
        void      (*packet_handler)(struct _CONN *);
        void      (*chunk_reader)(struct _CONN *);
        void      (*recv_chunk)(struct _CONN *, size_t);
        void      (*recv_file)(struct _CONN *, char *, long long , long long );
        int       (*push_chunk)(struct _CONN *, void *, size_t);
        int       (*push_file)(struct _CONN *, char *, long long , long long );
        void      (*data_handler)(struct _CONN *);
        void      (*oob_handler)(struct _CONN *);
        void      (*transaction_handler)(struct _CONN *, int tid);
        void	  (*push_message)(struct _CONN *, int);
        int       (*start_cstate)(struct _CONN *);
        void      (*over_cstate)(struct _CONN *);
        void      (*set_timeout)(struct _CONN *, long long timeout);
        void      (*close)(struct _CONN *); 
        void      (*terminate)(struct _CONN *); 
        void      (*clean)(struct _CONN **);
    } CONN;
    /* Initialize CONN */
    CONN *conn_init(char *ip, int port);
#endif

#ifdef __cplusplus
}
#endif

#endif
