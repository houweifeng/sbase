#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
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
#ifndef BUF_SIZE
/* 1M = 0x100000 */
#define	BUF_SIZE   0x100000 
#endif
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 	65535
#endif
#ifndef MAX_PACKET_LEN
#define MAX_PACKET_LEN		65535
#endif
#ifndef MAX_PROCTHREADS
#define MAX_PROCTHREADS		256
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

#ifndef SETRLIMIT
#define SETRLIMIT(NAME, RLIM, rlim_set)\
{\
	struct rlimit rlim;\
	rlim.rlim_cur = rlim_set;\
	rlim.rlim_max = rlim_set;\
	if(setrlimit(RLIM, (&rlim)) != 0) {\
		fprintf(stderr, "setrlimit RLIM[%s] cur[%ld] max[%ld] failed, %s\n",\
				NAME, rlim.rlim_cur, rlim.rlim_max, strerror(errno));\
		 _exit(-1);\
	} else {\
		fprintf(stdout, "setrlimit RLIM[%s] cur[%ld] max[%ld]\n",\
				NAME, rlim.rlim_cur, rlim.rlim_max);\
	}\
}
#define GETRLIMIT(NAME, RLIM)\
{\
	struct rlimit rlim;\
	if(getrlimit(RLIM, &rlim) != 0 ) {\
		fprintf(stderr, "getrlimit RLIM[%s] failed, %s\n",\
				NAME, strerror(errno));\
	} else {\
		fprintf(stdout, "getrlimit RLIM[%s] cur[%ld] max[%ld]\n", \
				NAME, rlim.rlim_cur, rlim.rlim_max);\
	}\
}
#endif

/* TYPES PREDEFINE */
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
struct _LOGGER;
struct _TIMER;
struct _BUFFER;
struct _QUEUE;
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
        uint64_t offset;
        uint64_t len;
	void *mutex;

        /* method */
        int (*set)(struct _CHUNK *, int , int , char *, uint64_t, uint64_t);
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
		"chunk->offset:%llu\n" \
		"chunk->len:%llu\n\n", \
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
#define S_STATE_READY		0x00
#define S_STATE_READ_CHUNK	0x02
#define S_STATE_WRITE_STATE     0x04
#define S_STATE_PACKET_HANDLING	0x08
#define S_STATE_DATA_HANDLING	0x10
#define S_STATE_CLOSE		0x12
#define S_STATES	(S_STATE_READY & S_STATE_READ_CHUNK & S_STATE_WRITE_STATE \
			S_STATE_PACKET_HANDLING & S_STATE_DATA_HANDLING & S_STATE_CLOSE )
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
        struct _QUEUE *message_queue;

	/* Running options */
	int running_status;
	uint32_t sleep_usec;

	/* Global options */
	struct _TIMER *timer;
	struct _LOGGER *logger;

        /* APIS */
        int  (*add_service)(struct _SBASE * , struct _SERVICE *);
	int  (*set_log)(struct _SBASE * , char *);
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

        /* Event options */
	EVBASE *evbase;
        EVENT  *event;

	/* Message Queue options for WORKING_PROC_MODE */
        struct _QUEUE *message_queue;

        /* Prothread options */
        int working_mode;
        int max_procthreads;
        int running_procthreads;
       	struct _PROCTHREAD **procthreads;
        struct _PROCTHREAD *procthread;

        /* Connection options */
        int max_connections;
        int running_connections;

        /* Packet options */
        int packet_type;
        int packet_length;
        char *packet_delimiter;
        int  packet_delimiter_length;
        uint32_t buffer_size;

        /* Global options */
        struct _TIMER *timer;
        struct _LOGGER *logger;
	char *logfile;

        /* Running options */
        int      running_status;
        uint32_t heartbeat_interval;
        uint32_t sleep_usec;
        uint32_t conn_timeout;

        /* Callback options */
        /* Heartbeat Handler */
        void (*cb_heartbeat_handler)(void *arg);
        /* Read From Buffer and return packet length to get */
        int (*cb_packet_reader)(const struct _CONN*, const struct _BUFFER *);
        /* Packet Handler */
        void (*cb_packet_handler)(const struct _CONN*, const struct _BUFFER *);
        /* Data Handler */
        void (*cb_data_handler)(const struct _CONN*, const struct _BUFFER *,
		 const struct _CHUNK *, const struct _BUFFER *);
        /* OOB Data Handler */
        void (*cb_oob_handler)(const struct _CONN *, const struct _BUFFER *oob);

        /* Methods */
        void (*event_handler)(int, short, void*);
        int  (*set)(struct _SERVICE * );
        void (*run)(struct _SERVICE * );
        void (*addconn)(struct _SERVICE *, int , struct sockaddr_in *);
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
        struct _QUEUE		*message_queue;

        /* Timer options */
        struct _TIMER		*timer;

        /* Logger option */
        struct _LOGGER		*logger;

        /* Methods */
        void (*run)(void *);
	void (*running_once)(struct _PROCTHREAD *);
        void (*addconn)(struct _PROCTHREAD *, struct _CONN *);
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
#define C_STATE_UNCONNECT 0x00
#define C_STATE_CONNECTED 0x01
#define C_STATE_FREE      0x02
#define C_STATE_USING     0x04
#define C_STATE_CLOSE     0x08
#define IP_MAX            16
#define CONN_NONBLOCK     1
#define CONN_SLEEP_USEC	  100u
#define CONN_IO_TIMEOUT   100000u
#define RECONNECT_MAX     10000
#ifndef CONN_MAX
#define CONN_MAX          131070
#endif
/* struct CONN */
typedef struct _CONN
{
	/* INET options */
	char			ip[IP_MAX];
	int			port;
	struct			sockaddr_in sa;
	int			fd;
	/* Transaction option */
	int			s_id;
	int			s_state;

	/* Packet options */
        int			packet_type;
        int 			packet_length;
        char 			*packet_delimiter;
        int  			packet_delimiter_length;
        uint32_t 		buffer_size;
	/* connection bytes stats */
	uint64_t		recv_oob_total;
	uint64_t		sent_oob_total;
	uint64_t		recv_data_total;
	uint64_t		sent_data_total;

	/* Global  options */
	//parent pointer 
	void			*parent;
	//message queue 
	struct _QUEUE           *message_queue;
	//buffer 
	struct _BUFFER		*buffer;
	//OOB 
	struct _BUFFER          *oob;
	//cache
	struct _BUFFER          *cache;
	//packet 
	struct _BUFFER		*packet;
	// chunk 
	struct _CHUNK		*chunk;
	// send queue 
	struct _QUEUE		*send_queue;
	// Logger
        struct _LOGGER          *logger;
	// Timer
        struct _TIMER           *timer;

	/* Event options */
	EVBASE 			*evbase;
	EVENT			*event;

	/* Callback */
	/* Read From Buffer and return packet length to get */
        int (*cb_packet_reader)(const struct _CONN*, const struct _BUFFER *);
        /* Packet Handler */
        void (*cb_packet_handler)(const struct _CONN*, const struct _BUFFER *);
        /* Data Handler */
        void (*cb_data_handler)(const struct _CONN*, const struct _BUFFER *,
		 const struct _CHUNK *, const struct _BUFFER *);
        /* OOB Data Handler */
        void (*cb_oob_handler)(const struct _CONN *, const struct _BUFFER *oob);
	
	/* Methods */
	int	  (*set)(struct _CONN *);
	void	  (*event_handler)(int, short, void *);
	void      (*read_handler)(struct _CONN *);
	void      (*write_handler)(struct _CONN *);
	int    	  (*packet_reader)(struct _CONN *);
	void      (*packet_handler)(struct _CONN *);
	void      (*chunk_reader)(struct _CONN *);
	void      (*recv_chunk)(struct _CONN *, size_t);
	void      (*recv_file)(struct _CONN *, char *, uint64_t, uint64_t);
	int       (*push_chunk)(struct _CONN *, void *, size_t);
	int       (*push_file)(struct _CONN *, char *, uint64_t, uint64_t);
	void      (*data_handler)(struct _CONN *);
	void      (*oob_handler)(struct _CONN *);
	void	  (*push_message)(struct _CONN *, int);
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
